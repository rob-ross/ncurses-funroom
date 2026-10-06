//  simple_mouse.c
//
//  Created by Rob Ross on 10/2/26.
//
//  Copyright (c) 2026.  All rights reserved.

//
#include <ncurses.h>

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // CRITICAL: This allows ncurses to capture special keys like KEY_MOUSE

    // 1. Disable click resolution delay (0 ms wait)
    //     this will allow display of mouse click location with no delay
    //      normally ncurses will wait for a period after the first click to see if it should generate a
    //      double-click or triple-click event. But this means it won't return a single mouse click
    //      until this wait time expires.
    mouseinterval(0);

    // 2. Reduce escape sequence delay (in milliseconds)
    //  Terminal emulators send mouse events as ANSI escape sequences beginning with the ESC byte (e.g., \033[<0;20;10M).
    //  When ncurses encounters an ESC character, it waits for ESCDELAY milliseconds (default is often 1000ms or
    //  100ms depending on OS/build) to verify whether the user just pressed the Escape key or if subsequent
    //  characters are following as part of an escape sequence.
    set_escdelay(25);


    // Enable tracking for standard left clicks (taps) and presses
    mousemask(BUTTON1_CLICKED | BUTTON1_PRESSED, NULL);

    printw("Click anywhere on the screen with your trackpad. Press 'q' to quit.\n");
    refresh();

    int ch;
    while ((ch = getch()) != 'q') {
        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                // Clear the line and print click coordinates
                move(2, 0);
                clrtoeol();
                printw("Trackpad Click Detected! X: %d, Y: %d, LINES:%d, COLS:%d", event.x, event.y, LINES, COLS);
                refresh();
            }
        }
    }
    endwin();
    return 0;
}
