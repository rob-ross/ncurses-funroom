#include <ncurses.h>

int main()
{
	initscr();

	attron(A_BOLD);
	addstr("Twinkle, twinkle little star\n");
	attron(A_BLINK);
	addstr("How I wonder what you are.\n");
	attroff(A_BOLD);
	addstr("Up above the world so high,\n");
	addstr("Like a diamond in the sky.\n");
	attrset(A_NORMAL);
	addstr("Twinkle, twinkle little star\n");
	addstr("How I wonder what you are.\n");
	refresh();
    // Loop until a genuine user keypress is detected (ignoring initial PTY resize events)
    int c;
    do {
        c = getch();
    } while (c == KEY_RESIZE || c == ERR);

	endwin();
	return 0;
}

