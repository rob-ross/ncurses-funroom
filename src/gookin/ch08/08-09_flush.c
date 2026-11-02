#include <ncurses.h>

int main()
{

	char buffer[81];

	initscr();

	addstr("Type. I'll wait...\n");
	refresh();
	napms(5000);			/* 5 seconds */

	addstr("Flushing buffer.\n");
	flushinp();  // flush input
	addstr("Here is what you typed:\n");
	getnstr(buffer,80);

    getch();

	endwin();
	return 0;
}

