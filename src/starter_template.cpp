//  starter_template.cpp
//
//  Created by Rob Ross on 10/20/26.
//
//  Copyright (c) 2026.  All rights reserved.

// simple starter template for ncurses experiments

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

int main_loop() {
    int c;


    timeout(-1); // read blocks
    mvwprintw(stdscr, 0,0,  "Press any key to exit...");
    c = getch();

    return 0;
}

int main() {
    init_locale();
    init_ncurses();

    int result = main_loop();

    end_ncurses();

    return result;
}
