#include <ncurses.h>

int main()
{
	WINDOW *second;

	initscr();
	start_color();

	/* configure colors */
	init_pair(1,COLOR_WHITE,COLOR_BLUE);
	init_pair(2,COLOR_WHITE,COLOR_RED);

	/* create the second window */
	second = newwin(0,0,0,0);
	if(second == NULL)
	{
		endwin();
		puts("Unable to create window");
		return 1;
	}
	wbkgd(second,COLOR_PAIR(2));
	waddstr(second,"This is the second window\n");

	/* standard screen */
	bkgd(COLOR_PAIR(1));
	addstr("This is the standard screen\n");
	addstr("Press Enter");
	refresh();
    int c;

    do {
        c = getch();
        if (c == KEY_RESIZE) {
            /* Resize second window to new terminal dimensions */
            wresize(second, LINES, COLS);
            wbkgd(second, COLOR_PAIR(2));
            werase(second);
            waddstr(second, "This is the second window\n");
        }
    } while (c == KEY_RESIZE || c == ERR);
	/* show second window */
	wrefresh(second);
    do {
        c = wgetch(second);

    } while (c == KEY_RESIZE || c == ERR);
	/* show standard screen again */
	touchwin(stdscr);
	refresh();
    do {
        c = getch();
    } while (c == KEY_RESIZE || c == ERR);

	endwin();
	return 0;
}
