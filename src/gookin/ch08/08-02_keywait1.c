#include <ncurses.h>

int main()
{
	int value = 0;

	initscr();

	addstr("Press any key to begin:\n");
	refresh();
	getch();

/* turn off getch() wait */
	nodelay(stdscr,TRUE);
	addstr("Press any key to stop the loop!\n");
    // 0==invisible, 1==slightly visible, 3== very visible
    // but in Terminal and Ghosttty, 0 will hide the cursor, but 1,2 displays the cursor set in the terminal
    // no visible difference between 1 and 2
    curs_set(2);
	while(getch() == ERR)
	{
		printw("%d\r",value++);
		refresh();
	}

	endwin();
	return 0;
}

