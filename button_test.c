#include <ncurses.h>
#include <stdio.h>
#include <string.h>


int main() {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);

    // Set up the button
    char* button_text = "Click Me!";
    int button_pos_x = 10;
    int button_pos_y = 5;
    int button_width = strlen(button_text) + 2;

    while (1) {
        clear();

        // Draw the button
        attron(A_REVERSE);
        mvprintw(button_pos_y, button_pos_x, " %-*s ", button_width - 2, button_text);
        attroff(A_REVERSE);

        refresh();

        // Wait for user input
        int ch = getch();

        // Handle mouse clicks
        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                if (event.y == button_pos_y && event.x >= button_pos_x && event.x < button_pos_x + button_width) {
                    // Button was clicked
                    // Add your button click logic here
                    printf("Button clicked bro\n");
                    fflush(0);
                    // break;
                }
            }
        }
    }

    // Clean up ncurses
    endwin();

    return 0;
}
