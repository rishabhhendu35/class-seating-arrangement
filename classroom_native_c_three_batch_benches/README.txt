CLASSROOMOS — NATIVE C SERVER VERSION (NO EMSCRIPTEN)
=====================================================

This version runs the C program as a small local HTTP server on Windows.
The webpage sends form values to C. C performs the seating allocation and
returns the generated HTML layout. No Emscripten or WebAssembly is needed.

FILES
-----
main.c       C program: local HTTP server + seating logic + HTML generation
index.html   User interface
style.css    Dashboard and classroom design
app.js       Sends the form to the C server and displays its HTML response

REQUIREMENTS
------------
- Windows
- GCC compiler (MinGW-w64 / MSYS2)
- VS Code is optional

COMPILE
-------
Open Command Prompt or the VS Code terminal in this project folder.
Run:

    gcc main.c -o classroom_server.exe -lws2_32

If compilation succeeds, classroom_server.exe will appear.

RUN
---
Run:

    classroom_server.exe

The terminal should say:
    Server is running at http://localhost:8080

Keep that terminal window open.

OPEN THE WEBSITE
----------------
Open Chrome and visit:

    http://localhost:8080

Enter rows, students, batches, teachers and teacher desk columns.
Click "Generate seating plan". The browser sends the values to the C server,
and C returns the classroom layout HTML.

STOP SERVER
-----------
Click the terminal window and press Ctrl+C.

IMPORTANT
---------
Keep main.c, index.html, style.css and app.js in the same folder as the
executable. Do not open index.html by double-clicking it; visit localhost:8080.
The server listens only on your own computer (127.0.0.1), not on the network.
Port 8080 must be free. If it is busy, change PORT near the top of main.c,
recompile, and use the matching port in the browser.


UPDATED SEATING RULES
- Each full bench seats 3 students.
- The 3 seats on a full bench are assigned to 3 different batches.
- Benches are placed side by side across each row; rows continue towards the back.
- At least 3 batches are required.
- If the student count is not divisible by 3, the final bench has 1 or 2 occupied seats.
- Native C + Windows Winsock project. No Emscripten or WebAssembly is used.
