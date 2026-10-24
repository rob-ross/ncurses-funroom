#include <ncurses.h>

int main()
{
	char text1[] = "This is the first line\n";
	char text2[] = "Line two here\n";
	char text3[] = "The third line\n";
	char text4[] = "Fourth line here\n";
	char text5[] = "And the fifth line\n";

	initscr();

	addstr(text1);
	addstr(text3);
	addstr(text5);
	refresh();
    // Loop until a genuine user keypress is detected (ignoring initial PTY resize events)
    int c;
    do {
        c = getch();
    } while (c == KEY_RESIZE || c == ERR);


	move(1,0);		/* Second line/row */
	insertln();		/* add a blank line */
	refresh();

    getch();

	endwin();
	return 0;
}

