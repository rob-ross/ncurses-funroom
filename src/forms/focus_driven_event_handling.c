//  focus_driven_event_handling.c
//
//  Created by Rob Ross on 11/12/26.
//
//  Copyright (c) 2026.  All rights reserved.

#define _XOPEN_SOURCE_EXTENDED 1  // Required for wide-character ncurses
#include <ncurses.h>


#include <ctype.h>
#include <string.h>
#include <locale.h>


typedef enum {
    FOCUS_MENU,
    FOCUS_INPUT
} FocusArea;

int main() {
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    // mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
    mousemask(ALL_MOUSE_EVENTS , nullptr);
    timeout( 50);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    WINDOW *menu_win = newwin(3, max_x, 0, 0);
    WINDOW *input_win = newwin(3, max_x, max_y - 3, 0);
    keypad(menu_win, TRUE);
    keypad(input_win, TRUE);

    FocusArea current_focus = FOCUS_INPUT;
    char buffer[256] = "";
    int buf_len = 0;
    int running = 1;

    while (running) {
        // Redraw windows based on state
        werase(menu_win);
        box(menu_win, 0, 0);
        mvwprintw(menu_win, 1, 2, "[Menu Area] %s",
                  (current_focus == FOCUS_MENU) ? "(FOCUSED - Use 'q' to quit)" : "");
        wrefresh(menu_win);

        werase(input_win);
        box(input_win, 0, 0);
        mvwprintw(input_win, 1, 2, "> %s", buffer);
        if (current_focus == FOCUS_INPUT) {
            // Position hardware cursor at insertion point
            wmove(input_win, 1, 4 + buf_len);
        }
        wrefresh(input_win);

        // Fetch single key or event
        int ch = wgetch((current_focus == FOCUS_INPUT) ? input_win : menu_win);

        // 1. Global Navigation & Focus Handling
        if (ch == '\t') {
            // Toggle focus with Tab
            current_focus = (current_focus == FOCUS_INPUT) ? FOCUS_MENU : FOCUS_INPUT;
            continue;
        }

        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_CLICKED) {
                    if (wenclose(input_win, event.y, event.x)) {
                        current_focus = FOCUS_INPUT;
                    } else if (wenclose(menu_win, event.y, event.x)) {
                        current_focus = FOCUS_MENU;
                    }
                }
            }
            continue;
        }

        // 2. Focused Input Handling
        if (current_focus == FOCUS_INPUT) {
            if (ch == '\n' || ch == KEY_ENTER) {
                // Submit command/text
                buffer[0] = '\0';
                buf_len = 0;
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (buf_len > 0) {
                    buffer[--buf_len] = '\0';
                }
            } else if (isprint(ch) && buf_len < sizeof(buffer) - 1) {
                buffer[buf_len++] = (char)ch;
                buffer[buf_len] = '\0';
            }
        } else if (current_focus == FOCUS_MENU) {
            if (ch == 'q' || ch == 'Q') {
                running = 0;
            }
        }
    }

    delwin(input_win);
    delwin(menu_win);
    endwin();
    return 0;
}
