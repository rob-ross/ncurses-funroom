//  02-05_yoda.c
//
//  Created by Rob Ross on 10/21/26.

#include <ncurses.h>

int main() {
    int yoda = 874;
    int ss = 65;

    initscr();
    printw("Yoda is %d years old\n", yoda);
    printw("He has collected %d years", yoda - ss);
    printw(" of Social Security.");
    refresh();
    // Loop until a genuine user keypress is detected (ignoring initial PTY resize events)
    int c;
    do {
        c = getch();  // does a wrefresh(stdscrn) before waiting for input
    } while (c == KEY_RESIZE || c == ERR);

    endwin();
    return (0);
}

//Excerpt From
//Dan Gookin's Guide to Ncurses Programming
//Dan Gookin
//This material may be protected by copyright.
