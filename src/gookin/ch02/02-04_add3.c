//  02-04_add3.c
//
//  Created by Rob Ross on 10/21/26.
//

#include <ncurses.h>

int main() {
    char t1[] = "Shall I compare thee";
    char t2[] = " to a summer's day?";

    initscr();
    addstr(t1); /* add the first string */
    addstr(t2); /* add the second string */
    move(2, 0); /* row 3, column 1 */
    addstr("Though art more lovely...");
    refresh(); /* display the result */
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
