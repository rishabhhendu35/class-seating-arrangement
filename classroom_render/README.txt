CLASSROOM SEATING ARRANGEMENT - RENDER DEPLOYMENT

This version uses POSIX sockets and is intended for Linux hosting such as Render.

Render Web Service settings:
Branch: main
Root Directory: leave blank (project files are in repository root)
Build Command: gcc -O2 -Wall -Wextra main.c -o classroom_server
Start Command: ./classroom_server

The server reads Render's PORT environment variable and listens on 0.0.0.0.
Keep index.html, style.css and app.js in the same directory as main.c.

For local Linux testing:
gcc -O2 -Wall -Wextra main.c -o classroom_server
./classroom_server
Then open http://localhost:8080
