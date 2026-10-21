//  resize_handling.cpp
//
//  Created by Rob Ross on 10/20/26.
//
//  Copyright (c) 2026.  All rights reserved.


#include <ncurses.h>

int main() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Partition the display into distinct, non-overlapping WINDOWs
    WINDOW* header_win = newwin(3, cols, 0, 0);
    WINDOW* main_win   = newwin(rows - 4, cols, 3, 0);
    WINDOW* status_win = newwin(1, cols, rows - 1, 0);

    keypad(status_win, TRUE);

    // Draw to discrete windows
    box(header_win, 0, 0);
    mvwprintw(header_win, 1, 2, "My Application Header");

    box(main_win, 0, 0);
    mvwprintw(main_win, 1, 2, "Main Area Content");

    mvwprintw(status_win, 0, 0, "Press 'q' to quit...");

    // Staged render
    wnoutrefresh(header_win);
    wnoutrefresh(main_win);
    wnoutrefresh(status_win);
    doupdate();

    // Event loop: listen for input on status_win (stdscr is never touched or refreshed)
    int ch;
    do {
        ch = wgetch(status_win);

        if (ch == KEY_RESIZE) {
            // 1. Get new terminal dimensions
            getmaxyx(stdscr, rows, cols);

            // 2. Clean up old windows
            delwin(header_win);
            delwin(main_win);
            delwin(status_win);

            // 3. Re-partition display with updated dimensions
            header_win = newwin(3, cols, 0, 0);
            main_win   = newwin(rows - 4, cols, 3, 0);
            status_win = newwin(1, cols, rows - 1, 0);
            keypad(status_win, TRUE);

            // 4. Redraw content
            box(header_win, 0, 0);
            mvwprintw(header_win, 1, 2, "My Application Header");

            box(main_win, 0, 0);
            mvwprintw(main_win, 1, 2, "Main Area Content");

            mvwprintw(status_win, 0, 0, "Press 'q' to quit...");

            // 5. Composite all windows in a single update pass
            wnoutrefresh(header_win);
            wnoutrefresh(main_win);
            wnoutrefresh(status_win);
            doupdate();
        }
    } while (ch != 'q' && ch != 'Q');

    delwin(header_win);
    delwin(main_win);
    delwin(status_win);
    endwin();

    return 0;
}
