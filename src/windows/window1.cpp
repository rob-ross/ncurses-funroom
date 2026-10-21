//  window1.cpp
//
//  Created by Rob Ross on 10/20/26.
//
//  Copyright (c) 2026.  All rights reserved.


#include <iostream>
#include <locale>
#include <ncurses.h>

void init_ncurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // CRITICAL: This allows ncurses to capture special keys like KEY_MOUSE
}

void init_locale() {
    // Sets the global C++ locale to the user-configured system environment locale ("")
    // and synchronizes the underlying C library locale as well.
    std::locale::global(std::locale(""));

    // To ensure pre-existing standard I/O streams yield optimal results with wide characters:
    std::wcout.imbue(std::locale());
    std::wcin.imbue(std::locale());
}

void end_ncurses() {
    endwin();
}

/*
 *  Draws to stdscr, creates second window with same size, draws to that screen.
 *  result: no output visible from second window.
 *
 *
 */
int test1() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols); // get current window rows, columns
    mvwprintw(stdscr, 4, 0, "Printing to stdscr"); // this results in the text being displayed. No refresh was required.

    // let's try a second window with the same size as the stdscr.

    WINDOW* win_2 = newwin(rows, cols, 0, 0);
    mvwprintw(win_2, 6, 0, "Printing to win_2"); // ah, interesting. This text does not show up!!!!

    // will a refresh make the text from win_2 visible?
    wrefresh(win_2); // no, this does not make the text visible

    // what about refreshing the stdscr?
    // wrefresh(stdscr); // no, still nothing. Perhaps because they are the same size and obscure one another?

    // delwin(win_2);

    return 0;
}

int main_loop() {

    test1();



    int c;
    timeout(-1); // makes the read block for input
    mvwprintw(stdscr, 10, 0,  "Press any key to exit...");

    //todo temp test so stdscrn is not marked dirty:
    untouchwin(stdscr);

    // We don't want to count a resize event as a key down event in any case, but
    // when running a pseudo-terminal in CLion, it will be resized to fit the display panel for
    // the terminal, causing an initial resize key event. This results in the program immediately exiting
    // as if the user had actually pressed a key.
    //
    // Loop until a genuine user keypress is detected (ignoring initial PTY resize events)
    do {
        c = getch();  // does a wrefresh(stdscrn) before waiting for input
    } while (c == KEY_RESIZE || c == ERR);

    return 0;
}

int main() {
    init_locale();
    init_ncurses();

    int result = main_loop();

    end_ncurses();

    return result;
}
