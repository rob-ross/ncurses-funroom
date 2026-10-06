//  wide.c
//
//  Created by Rob Ross on 10/2/26.
//
//  Copyright (c) 2026.  All rights reserved.

//
// this _XOPEN_SOURCE_EXTENDED 1 #define has to come before including ncurses.h
// this enables wide-char functions in ncurses on Mac OS.
#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <wchar.h>
#include <locale.h>

int main() {
    setlocale(LC_ALL, ""); // Initializes the system's native UTF-8 locale

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // CRITICAL: This allows ncurses to capture special keys like KEY_MOUSE
    // U+25B2 : up-pointing triangle. \uXXXX or \UXXXXXX are Universal Character Names (UCN)
    const char * triangle = "\u25B2";
    const char * smile_emoji = "\U0001F600";
    // we're printing to stdout during a ncurses app. What will happen??
    // nothing happens.
    printf("printf: triangle: %s, smile emoji:%s\n", triangle, smile_emoji);
    fflush(stdout);


    wprintw(stdscr,"wprintw: triangle: %s, smile emoji:%s\n", triangle, smile_emoji);
    waddstr(stdscr, triangle);
    waddstr(stdscr, smile_emoji);

    wprintw(stdscr, "\nEnter a wide char:");
    refresh();
    wint_t wch;
    int status = get_wch(&wch);
    if (status == OK) {
        // %lc prints a wchar_t / wint_t wide character
        // %X prints its hex codepoint
        wprintw(stdscr, "\n\nYou entered: %lc (Codepoint: U+%04X, Decimal: %d)\n", wch, (unsigned int)wch, (int)wch);
    } else if (status == KEY_CODE_YES) {
        wprintw(stdscr, "\n\nYou pressed a function/special key (code: %d)\n", (int)wch);
    } else {
        wprintw(stdscr, "\n\nError reading input.\n");
    }

    wprintw(stdscr, "\nPress any key to exit...");
    refresh();

    // Wait for one more wide character keypress before exiting
    get_wch(&wch);


    endwin();
    return 0;
}

