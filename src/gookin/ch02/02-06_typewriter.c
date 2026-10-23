#include <ncurses.h>

int main()
{
	int ch;

	initscr();
	addstr("Type a few lines of text\n");
	addstr("Press ~ to quit\n");
	refresh();

	while( (ch = getch()) != '~')
		;

	endwin();
    printf("to quit, you pressed: %d\n", ch);
	return 0;
}

