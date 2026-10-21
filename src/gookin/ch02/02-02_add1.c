//  02-02_add1.c
//
//  Created by Rob Ross on 10/21/26.
//
#include <ncurses.h>

int main() {
    char text[] = "Greetings from Ncurses!";
    char *t;

    initscr(); /* initialize Ncurses */
    t = text; /* initialize the pointer */

    while (*t) /* loop through the string */
    {
        addch(*t); /* one char output */
        t++; /* increment the pointer */
        refresh(); /* update the screen */
        napms(100); /* delay .1 sec */
    }

    // Loop until a genuine user keypress is detected (ignoring initial PTY resize events)
    int c;
    do {
        c = getch();  // does a wrefresh(stdscrn) before waiting for input
    } while (c == KEY_RESIZE || c == ERR);

    endwin(); /* clean up Ncurses */
    return (0);
}

// Excerpt From
// Dan Gookin's Guide to Ncurses Programming
// Dan Gookin
// This material may be protected by copyright.
