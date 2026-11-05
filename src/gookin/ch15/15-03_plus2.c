#include <ncurses.h>

#define HLIN 10
#define VLIN 5

int main()
{
    int thickness = 5;

	int c;

	initscr();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int half_row = rows / 2;
    int half_col = cols / 2;

    int h_top_y = half_row - thickness;
    int h_bot_y = half_row + thickness;

    int v_left_x  = half_col - thickness*2;
    int v_right_x = half_col + thickness*2;


    // h lines
    mvhline(h_top_y, 0, 0, cols);
    mvhline(h_bot_y, 0, 0, cols);

    // v lines
    mvvline(0, v_left_x, 0, rows);
    mvvline(0, v_right_x, 0, rows);

    // clear center square
    mvhline(h_top_y, v_left_x + 1, ' ', v_right_x - v_left_x - 1);
    mvhline(h_bot_y, v_left_x + 1, ' ', v_right_x - v_left_x - 1);

    mvvline(h_top_y, v_left_x, ' ',  h_bot_y - h_top_y + 1);
    mvvline(h_top_y, v_right_x, ' ',  h_bot_y - h_top_y + 1);

    curs_set(0);


	refresh();
	getch();

	endwin();
	return 0;
}
