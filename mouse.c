#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>



#include <fcntl.h>


#define ESC "\x1b"

void enableMouseEvents() {
    // Request mouse tracking
    printf("%s[?1000h", ESC);
    fflush(stdout);

    // Enable button press/release tracking
    printf("%s[?1003h", ESC);
    fflush(stdout);
}

void disableMouseEvents() {
    // Disable button press/release tracking
    printf("%s[?1003l", ESC);
    fflush(stdout);

    // Disable mouse tracking
    printf("%s[?1000l", ESC);
    fflush(stdout);
}

int main() {
    enableMouseEvents();

    // Clear screen and draw button
    printf("%s[2J", ESC);
    printf("%s[%d;%dH", ESC, 5, 10);
    printf("Click Me!");
    fflush(stdout);

    // Set terminal to non-blocking mode
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    // Capture mouse events
    char buf[32];
    int bytesRead;
    int buttonClicked = 0;
    while (!buttonClicked) {
        bytesRead = read(STDIN_FILENO, buf, sizeof(buf));
        if (bytesRead > 0) {
            for (int i = 0; i < bytesRead; i++) {
                if (buf[i] == '\e' && i + 2 < bytesRead && buf[i + 1] == '[' && buf[i + 2] == 'M') {
                    int button = buf[i + 3] - ' ';
                    int x = buf[i + 4] - '!' - 1;
                    int y = buf[i + 5] - '!' - 1;
                    if (button == 0 && x >= 10 && x < 21 && y == 5) {
                        buttonClicked = 1;
                        break;
                    }
                }
            }
        }
    }

    disableMouseEvents();

    // Clear screen and display button click message
    printf("%s[2J", ESC);
    printf("%s[%d;%dH", ESC, 10, 10);
    printf("Button Clicked!");
    fflush(stdout);

    // Wait for a moment
    sleep(1);

    return 0;
}
