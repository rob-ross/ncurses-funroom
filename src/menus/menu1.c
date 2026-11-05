//  menu1.c
//
//  Created by Rob Ross on 11/4/26.
//
//  Copyright (c) 2026.  All rights reserved.

#define _XOPEN_SOURCE_EXTENDED 1  // Required for wide-character ncurses
#include <ncurses.h>
#include <panel.h>

#include <locale.h>

/*
 *
*+-------------------------------------------------------------+  <- Top Bar Window (Static)

|  File    Edit    View    Help                               |
+-------------------------------------------------------------+

| +--------------+                                            |
| | New      ^N  | <- Dropdown Window (Wrapped in a Panel)    |
| | Open...  ^O  |    Set to high Z-index; Hidden by default.  |
| | Save     ^S  |                                            |
| +--------------+                                            |
|                                                             |
|                                                             |
| Main Content Window / Subwindow (Wrapped in a Panel)        |
| (Everything underneath the dropdown is perfectly preserved)|
+-------------------------------------------------------------+
 *
 *
 */

WINDOW* menubar = nullptr;
WINDOW* main_win = nullptr;

int main() {
    setlocale(LC_ALL, "");

    initscr();
    noecho();
    // cbreak();

    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_MAGENTA);

    // create windows
    menubar = newwin(1, cols, 0, 0); // 1 row tall by screen-width wide
    wbkgd(menubar,COLOR_PAIR(1));
    wattrset(menubar, COLOR_PAIR(1));
    keypad(menubar, true); // required to get mouse clicks
    mousemask( ALL_MOUSE_EVENTS, nullptr);

    main_win = newwin(rows - 1, cols, 1, 0); // rest of the screen
    wbkgd(main_win,COLOR_PAIR(2) + '.');
    wattrset(main_win, COLOR_PAIR(2) );
    mvwprintw(main_win, 0,0, "This is some sample text.\nType any key to continue...");


    PANEL * base_panel = new_panel(main_win);

    char * menu_titles[4] = { "File", "Edit", "View", "Help"};
    wmove(menubar, 0, 1);
    for (int i = 0; i < 4; ++i) {
        wprintw(menubar, "%s  ", menu_titles[i]);
    }

    char * menu_file_titles[3] = { "New", "Open...", "Save"};
    WINDOW* file_menu = newwin(5, 10, 1, 0);
    wbkgd(file_menu,COLOR_PAIR(1));
    box(file_menu, 0, 0);
    for (int i = 0; i < 3; ++i) {
        mvwprintw(file_menu, i + 1, 1, " %s", menu_file_titles[i]);
    }

    PANEL*  file_panel = new_panel(file_menu);
    hide_panel(file_panel);

    curs_set(0);

    update_panels();
    wnoutrefresh(menubar);
    wnoutrefresh(main_win);
    doupdate();

    getch();
    int c;
    do {
        c = wgetch(menubar);
        if (c != ERR) {
            if (c == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    // Clear the line and print click coordinates
                    wmove(main_win, 2, 0);
                    wclrtobot(main_win);
                    wprintw(main_win, "Trackpad Click Detected! X: %d, Y: %d, LINES:%d, COLS:%d\n", event.x, event.y, LINES, COLS);
                    if (event.y == 0 && event.x > 0 && event.x < 5) {
                        wprintw(main_win, "\nYou clicked on the File menu title.");
                        show_panel(file_panel);
                        top_panel(file_panel);
                    } else {
                        hide_panel(file_panel);
                    }
                    // wrefresh(main_win);
                    update_panels();
                    doupdate();
                }
            }
        }

    } while ( c != 'q');




    del_panel(base_panel); // destroys panel but not underlying window
    del_panel(file_panel);

    delwin(file_menu);
    delwin(menubar);
    delwin(main_win);

    endwin();
    return 0;

}
