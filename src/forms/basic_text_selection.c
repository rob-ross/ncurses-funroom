//  basic_text_selection.c
//
//  Created by Rob Ross on 11/12/26.
//
//  Copyright (c) 2026.  All rights reserved.

#include <ncurses.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>

int main(void) {
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Completely disable mouse tracking
    mousemask(0, NULL);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    WINDOW *input_win = newwin(3, max_x, max_y - 3, 0);
    keypad(input_win, TRUE);

    char buffer[256] = "Type here and try selecting with the mouse...";
    int buf_len = (int)strlen(buffer);

    werase(input_win);
    box(input_win, 0, 0);
    mvwprintw(input_win, 1, 2, "> %s", buffer);
    wmove(input_win, 1, 4 + buf_len);
    wrefresh(input_win);

    // Standard wgetch() - click and drag now selects natively in the terminal
    int ch = wgetch(input_win);

    delwin(input_win);
    endwin();
    return 0;
}

//
