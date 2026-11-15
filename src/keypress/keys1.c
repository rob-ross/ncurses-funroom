//  keys1.c
//
//  Created by Rob Ross on 11/11/26.
//
//  Copyright (c) 2026.  All rights reserved.

#include <ncurses.h>
#include <locale.h>

#define CTRL(x) ( (x) & 0x1F)

void print_state(bool raw_mode, bool keypad_mode, int row) {
    wmove(stdscr, row, 0);
    wclrtobot(stdscr);
    mvwprintw(stdscr, row, 0, "keypad: %s", keypad_mode ? "true" : "false");
    mvwprintw(stdscr, row + 1, 0, "raw mode: %s", raw_mode ? "raw" : "cbreak");
    wrefresh(stdscr);
}

int main() {
    setlocale(LC_ALL, "");

    initscr();
    bool raw_mode = true;
    raw();
    noecho();
    curs_set(0); // hide cursor

    bool keypad_mode = true;
    keypad(stdscr, keypad_mode);

    int c;
    wmove(stdscr, 1, 0);
    wprintw(stdscr, "Press F1 to quit.\nPress F2 to toggle keypad mode.\nPress F3 to toggle raw/cbreak mode.\n");
    print_state(raw_mode, keypad_mode, 10);
    while ( (c=getch()) != KEY_F(1) ) {
        // We need two switch statements because the value of ch for the same key changes based on the value of
        // keypad_mode. Actually, what happens when keypad_mode is false is that man keys on the keyboard are
        // encoded as an ANSI escape sequence, a string of chars, starting with the esc key.
        // But since getch() just reads those sequences one character at a time, the last byte read is the one
        // we switch on in the second switch statement.
        if (keypad_mode) {
            switch (c) {
                case KEY_F(2):
                    keypad_mode = !keypad_mode;
                    keypad(stdscr, keypad_mode);
                    break;
                case KEY_F(3):
                    raw_mode = !raw_mode;
                    if (raw_mode) {
                        nocbreak();
                        raw();
                    } else {
                        noraw();
                        cbreak();
                    }
                    break;
                default:
                    wmove(stdscr, 5, 0);
                    wclrtoeol(stdscr);
                    mvwprintw(stdscr, 5, 0, "You pressed '%s' : 0x%.2X, (dec %d), (ch & 0x1F)= 0x%.2X",
                        keyname(c), c, c, (c & 0x1F));
            }
        } else {
            switch (c) {
                case 0x50: // P : the actual 3 byte sequence is ESC-O-P.
                    goto END;
                case 0x51: // Q
                    keypad_mode = !keypad_mode;
                    keypad(stdscr, keypad_mode);
                    break;
                case 0x52:  // R
                    raw_mode = !raw_mode;
                    if (raw_mode) {
                        nocbreak();
                        raw();
                    } else {
                        noraw();
                        cbreak();
                    }
                    break;
                default:
                    wmove(stdscr, 5, 0);
                    wclrtoeol(stdscr);
                    mvwprintw(stdscr, 5, 0, "You pressed '%s' : 0x%.2X", keyname(c), c);
            }
        } // if (keypad_mode)

        print_state(raw_mode, keypad_mode, 10);

        // shows how we can differentiate between a key press and one with control + keypress
        if (c == 'a') {
            mvwprintw(stdscr, 11, 0, "You pressed 'a'");
        } else if (c == CTRL('a')) {
            mvwprintw(stdscr, 11, 0, "You pressed '^A'");
        }

    } // while ( (c=getch()) != KEY_F(1) )

END:
    endwin();

}
