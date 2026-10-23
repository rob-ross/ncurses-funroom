#include <ncurses.h>

int main()
{
	initscr();

	start_color();
	init_pair(1,COLOR_WHITE,COLOR_BLUE);
	bkgd(COLOR_PAIR(1) |'.');
	addstr("This is white text");
	refresh();
    // Loop until a genuine user keypress is detected (ignoring initial PTY resize events)
    int c;
    do {
        c = getch();
    } while (c == KEY_RESIZE || c == ERR);

	endwin();
	return 0;
}
