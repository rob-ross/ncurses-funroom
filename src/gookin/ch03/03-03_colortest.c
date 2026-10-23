
#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>

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
