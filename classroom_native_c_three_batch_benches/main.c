#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#define PORT 8080
#define REQ_SIZE 65536

static void send_response(SOCKET client, const char *status, const char *type, const char *body) {
    char header[512];
    int body_len = (int)strlen(body);
    snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n",
        status, type, body_len);
    send(client, header, (int)strlen(header), 0);
    send(client, body, body_len, 0);
}

static void url_decode(char *dst, const char *src, size_t cap) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 1 < cap; i++) {
        if (src[i] == '+') dst[j++] = ' ';
        else if (src[i] == '%' && src[i+1] && src[i+2]) {
            char hex[3] = {src[i+1], src[i+2], '\0'};
            dst[j++] = (char)strtol(hex, NULL, 16);
            i += 2;
        } else dst[j++] = src[i];
    }
    dst[j] = '\0';
}

static int get_form_int(const char *body, const char *key, int fallback) {
    char search[64];
    snprintf(search, sizeof(search), "%s=", key);
    const char *p = strstr(body, search);
    if (!p) return fallback;
    p += strlen(search);
    char value[32];
    size_t n = 0;
    while (p[n] && p[n] != '&' && n < sizeof(value)-1) {
        value[n] = p[n];
        n++;
    }
    value[n] = '\0';
    char decoded[32];
    url_decode(decoded, value, sizeof(decoded));
    return atoi(decoded);
}

static int valid(int rows, int students, int batches, int teachers, int cols) {
    return rows >= 1 && rows <= 30 &&
           students >= 1 && students <= 500 &&
           batches >= 3 && batches <= 20 &&
           teachers >= 0 && teachers <= 30 &&
           cols >= 1 && cols <= 10;
}

static char *make_layout(int rows, int students, int batches, int teachers, int cols) {
    if (!valid(rows, students, batches, teachers, cols)) {
        const char *msg = "<div class='error-box'>Please enter valid values: rows 1-30, students 1-500, batches 3-20, teachers 0-30 and teacher columns 1-10. At least 3 batches are required so every full bench can seat 3 different batches.</div>";
        char *copy = (char *)malloc(strlen(msg) + 1);
        if (copy) strcpy(copy, msg);
        return copy;
    }

    size_t cap = 420000, used = 0;
    char *out = (char *)malloc(cap);
    if (!out) return NULL;

    int total_benches = (students + 2) / 3;
    int bench_number = 0;
    int student_number = 1;

    used += (size_t)snprintf(out + used, cap - used,
        "<div class='classroom-result'>"
        "<div class='room-heading'><div><span class='room-kicker'>GENERATED FLOOR PLAN</span>"
        "<h2>Classroom seating map</h2><p>%d students · %d bench rows · %d batches · 3 students per bench</p></div>"
        "<span class='generated-chip'>C GENERATED</span></div>"
        "<div class='front-area'><div class='wall-label'>FRONT WALL</div>"
        "<div class='blackboard'>CLASSROOM BOARD <small>LEARN · CREATE · GROW</small></div>",
        students, rows, batches);

    if (teachers > 0) {
        used += (size_t)snprintf(out + used, cap - used,
            "<div class='faculty-label'>FACULTY AREA</div>"
            "<div class='teacher-grid' style='--teacher-cols:%d'>", cols);
        for (int t = 1; t <= teachers; t++) {
            used += (size_t)snprintf(out + used, cap - used,
                "<div class='teacher-desk'><span>TEACHER</span><strong>%02d</strong></div>", t);
        }
        used += (size_t)snprintf(out + used, cap - used, "</div>");
    }

    used += (size_t)snprintf(out + used, cap - used,
        "<div class='aisle-label'>— STUDENTS FACE THE BOARD —</div></div><div class='seating-area'>");

    for (int r = 1; r <= rows && bench_number < total_benches; r++) {
        int remaining_benches = total_benches - bench_number;
        int rows_left = rows - r + 1;
        int benches_this_row = (remaining_benches + rows_left - 1) / rows_left;

        used += (size_t)snprintf(out + used, cap - used,
            "<div class='desk-row'><div class='row-tag'>ROW <b>%02d</b></div><div class='desk-list'>", r);

        for (int b = 0; b < benches_this_row && bench_number < total_benches; b++) {
            used += (size_t)snprintf(out + used, cap - used,
                "<div class='three-seat-bench'><div class='bench-caption'>BENCH %02d</div><div class='bench-seats'>",
                bench_number + 1);

            for (int seat = 0; seat < 3 && student_number <= students; seat++) {
                int batch = ((bench_number + seat) % batches) + 1;
                used += (size_t)snprintf(out + used, cap - used,
                    "<div class='student-desk batch-%d'><div class='chair'></div>"
                    "<div class='desk-top'><strong>S%03d</strong><span>BATCH %d</span></div></div>",
                    batch, student_number, batch);
                student_number++;
            }

            used += (size_t)snprintf(out + used, cap - used, "</div></div>");
            bench_number++;
        }
        used += (size_t)snprintf(out + used, cap - used, "</div></div>");
    }

    used += (size_t)snprintf(out + used, cap - used,
        "</div><div class='back-wall'>BACK OF CLASSROOM</div><div class='batch-legend'><b>BATCH KEY</b>");
    for (int b = 1; b <= batches; b++) {
        used += (size_t)snprintf(out + used, cap - used,
            "<span><i class='legend-dot batch-%d'></i>Batch %d</span>", b, b);
    }
    used += (size_t)snprintf(out + used, cap - used, "</div></div>");
    return out;
}

static char *read_file(const char *name, long *length) {
    FILE *fp = fopen(name, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    char *data = (char *)malloc((size_t)size + 1);
    if (!data) { fclose(fp); return NULL; }
    size_t got = fread(data, 1, (size_t)size, fp);
    data[got] = '\0';
    fclose(fp);
    *length = (long)got;
    return data;
}

static void send_file(SOCKET client, const char *filename, const char *mime) {
    long length = 0;
    char *data = read_file(filename, &length);
    if (!data) {
        send_response(client, "404 Not Found", "text/plain", "File not found. Keep index.html, style.css and app.js beside classroom_server.exe.");
        return;
    }
    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\nContent-Type: %s; charset=utf-8\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n",
        mime, length);
    send(client, header, (int)strlen(header), 0);
    send(client, data, (int)length, 0);
    free(data);
}

static void handle_client(SOCKET client) {
    char request[REQ_SIZE];
    int total = 0;
    int received;
    request[0] = '\0';

    /* Read until the complete HTTP headers and request body arrive. */
    while (total < REQ_SIZE - 1) {
        received = recv(client, request + total, REQ_SIZE - 1 - total, 0);
        if (received <= 0) break;
        total += received;
        request[total] = '\0';

        char *header_end = strstr(request, "\r\n\r\n");
        if (header_end) {
            int header_len = (int)(header_end + 4 - request);
            int content_length = 0;
            char *cl = strstr(request, "Content-Length:");
            if (cl && cl < header_end) content_length = atoi(cl + 15);
            if (total >= header_len + content_length) break;
        } else if (strstr(request, "\r\n\r\n") == NULL && total > 16000) {
            break;
        }
    }

    if (strncmp(request, "GET / ", 6) == 0 || strncmp(request, "GET /index.html", 15) == 0) {
        send_file(client, "index.html", "text/html");
    } else if (strncmp(request, "GET /style.css", 14) == 0) {
        send_file(client, "style.css", "text/css");
    } else if (strncmp(request, "GET /app.js", 11) == 0) {
        send_file(client, "app.js", "application/javascript");
    } else if (strncmp(request, "POST /generate", 14) == 0) {
        char *header_end = strstr(request, "\r\n\r\n");
        if (!header_end) {
            send_response(client, "400 Bad Request", "text/plain", "Bad request");
        } else {
            char *body = header_end + 4;
            int rows = get_form_int(body, "rows", 0);
            int students = get_form_int(body, "students", 0);
            int batches = get_form_int(body, "batches", 0);
            int teachers = get_form_int(body, "teachers", 0);
            int cols = get_form_int(body, "teacherCols", 0);
            char *layout = make_layout(rows, students, batches, teachers, cols);
            if (layout) {
                send_response(client, "200 OK", "text/html", layout);
                free(layout);
            } else {
                send_response(client, "500 Internal Server Error", "text/plain", "Could not generate seating layout.");
            }
        }
    } else {
        send_response(client, "404 Not Found", "text/plain", "Page not found");
    }
}

int main(void) {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr;

    printf("============================================\n");
    printf("       CLASSROOMOS - C SEATING SERVER\n");
    printf("============================================\n");

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock initialisation failed.\n");
        return 1;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Could not create server socket.\n");
        WSACleanup();
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Port %d is busy. Close another server or change PORT in main.c.\n", PORT);
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR) {
        printf("Could not start listening.\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("\nServer is running at http://localhost:%d\n", PORT);
    printf("Keep this window open while using the website.\n");
    printf("Press Ctrl+C to stop the server.\n\n");

    while ((client_socket = accept(server_socket, NULL, NULL)) != INVALID_SOCKET) {
        handle_client(client_socket);
        closesocket(client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
