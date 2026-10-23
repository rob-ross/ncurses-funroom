//  color1.c
//
//  Created by Rob Ross on 10/22/26.
//

#define _XOPEN_SOURCE_EXTENDED 1
#include <wchar.h>
#include <ncursesw/ncurses.h>
// #include <ncurses.h>

int main()
{
    initscr();

    /* test for color ability of the terminal */
    if(!has_colors())
    {
        endwin();	/* exit Ncurses */
        puts("Terminal cannot do colors");
        return 1;
    }

    /* initialize Ncurses colors */
    if(start_color() != OK)
    {
        endwin();
        puts("Unable to start colors.");
        return 1;
    }

    if (can_change_color()) {
        // Redefine color index 15 to custom shade (RGB: 800, 200, 500)
        init_extended_color(15, 800, 200, 500);
        init_extended_pair(257, COLOR_BLACK, 15);

        // Prepare a complex character struct (cchar_t) holding the extended pair
        cchar_t bg;
        setcchar(&bg, (const wchar_t *)L" ", A_NORMAL, 257, nullptr);
        // Apply the background to stdscr
        wbkgrnd(stdscr, &bg);
    }

    /* colors are okay; continue */
    printw("Colors initialized.\n");
    printw("%d colors available.\n",COLORS);
    printw("%d color pairs.",COLOR_PAIRS);
    refresh();
    // Loop until a genuine user keypress is detected (ignoring initial PTY resize events)
    int c;
    do {
        c = getch();
    } while (c == KEY_RESIZE || c == ERR);

    endwin();
    return 0;
}
