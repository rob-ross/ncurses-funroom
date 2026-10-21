//  resize_newwin.cpp
//
//  Created by Rob Ross on 10/20/26.
//
//  Copyright (c) 2026.  All rights reserved.

#include <ncurses.h>

struct AppLayout {
    WINDOW* header;
    WINDOW* main;
    WINDOW* status;
};

// 1. Clean allocation
AppLayout create_layout() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    AppLayout layout;
    layout.header = newwin(3, cols, 0, 0);
    layout.main   = newwin(rows - 4, cols, 3, 0);
    layout.status = newwin(1, cols, rows - 1, 0);

    keypad(layout.status, TRUE);
    return layout;
}

// 2. Clean deallocation
void destroy_layout(AppLayout& layout) {
    if (layout.header) delwin(layout.header);
    if (layout.main)   delwin(layout.main);
    if (layout.status) delwin(layout.status);
}

// 3. Render contents
void draw_ui(const AppLayout& layout) {
    // Optional: Clear physical screen on geometry change to wipe old trails
    // erase(); wnoutrefresh(stdscr);

    box(layout.header, 0, 0);
    mvwprintw(layout.header, 1, 2, "My Application Header");

    box(layout.main, 0, 0);
    mvwprintw(layout.main, 1, 2, "Main Area Content");

    mvwprintw(layout.status, 0, 0, "Press 'q' to quit...");

    wnoutrefresh(layout.header);
    wnoutrefresh(layout.main);
    wnoutrefresh(layout.status);
    doupdate();
}

int main() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);

    AppLayout layout = create_layout();
    draw_ui(layout);

    int ch;
    while ((ch = wgetch(layout.status)) != 'q' && ch != 'Q') {
        if (ch == KEY_RESIZE) {
            // Re-partition smoothly on terminal resize
            destroy_layout(layout);
            layout = create_layout();
            draw_ui(layout);
        }
    }

    destroy_layout(layout);
    endwin();
    return 0;
}
