#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define ESC "\x1b"

void clearScreen() {
    printf("%s[2J", ESC);
}

void setCursorPosition(int x, int y) {
    printf("%s[%d;%dH", ESC, y, x);
}

void setForegroundColor(int color) {
    printf("%s[38;5;%dm", ESC, color);
}

void setBackgroundColor(int color) {
    printf("%s[48;5;%dm", ESC, color);
}

void resetColors() {
    printf("%s[0m", ESC);
}

void drawButton(int x, int y, int width, const char* label) {
    setCursorPosition(x, y);
    setForegroundColor(0);      // Set text color to black
    setBackgroundColor(15);    // Set background color to whites
    printf(" %-*s ", width-2, label);
    resetColors();
}

int getch() {
    struct termios oldTerm, newTerm;
    int ch;

    tcgetattr(STDIN_FILENO, &oldTerm);
    newTerm = oldTerm;
    newTerm.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);

    return ch;
}

int main() {
    const int buttonX = 10;
    const int buttonY = 5;
    const int buttonWidth = 12;
    const char* buttonText = "Click Me!";

    clearScreen();
    drawButton(buttonX, buttonY, buttonWidth, buttonText);
    fflush(stdout);


    printf("%s[?1000h", ESC);
    fflush(stdout);

    // Wait for user input
    int input;
    do {
        input = getch();
        printf("%x\n",input);
        fflush(0);
        if (input == '\x1b') {
            // Button is clicked
            
            clearScreen();
            setCursorPosition(buttonX, buttonY);
            printf("Screen Clicked!\n");
            break;
        }
    } while (input != 'q');



    printf("%s[?1000l", ESC);
    fflush(stdout);
    return 0;
}
