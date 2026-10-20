//  simple_animate1.cpp
//
//  Created by Rob Ross on 10/19/26.
//
//  Copyright (c) 2026.  All rights reserved.

//
/*
 *  light screen design
 *  bottom most row for info display
 *  lower-left : status of last operation
 *  lower-right: rows:cols of screen
 *  rows - 2 : left : prompt message, mode message
 *  rows - 3: left : prompt area for text entry
 *
 */


#include <locale>
#include <iostream>
#include <numbers>

#include <ncurses.h>

// display prompt in lower left of screen
void display_prompt(const char * prompt) {
    int rows, cols;
    getmaxyx(stdscr,rows,cols);  // get current window rows, columns
    move(rows - 2, 0);
    // int result = result = clrtoeol();
    printw(prompt);
}

// display the current number of rows and columns in the bottom-left of the screen
void display_rows_cols() {
    constexpr size_t buf_size = 10;
    char buf[buf_size]{};

    int rows, cols;
    getmaxyx(stdscr,rows,cols);  // get current window rows, columns
    int result;
    result = snprintf(buf, buf_size, "%4d:%-4d", rows, cols);

    //allow a max of 9 col spaces for display. This allows printing of `XXXX:YYYY`, accommodating max `9999:9999`
    int field_width = buf_size - 1;
    field_width += 2; // todo temp for vertica bars

    move(rows - 1, cols - field_width);
    // result = clrtoeol();
    printw("|%s|", buf);
}

void display_bottom_info(int rows, int cols) {
    // clear from last two lines of window down
    move(rows - 2, 0);
    int ret = clrtobot();
    display_rows_cols();
    display_prompt("Press 'q' to quit.");
}

int plot_graph(int cur_graph_col) {
    int rows, cols;
    static char plot_symbol = '.'; // so we can alternate each full period between x and -
    getmaxyx(stdscr,rows,cols);  // get current window rows, columns
    if (cur_graph_col > cols - 1) {
        cur_graph_col = 0;
        if (plot_symbol == '.') {
            plot_symbol = '-';
        } else {
            plot_symbol = '.';
        }
    }

    double step_size = (2 * std::numbers::pi) / (double)cols;

    double graph_x = cur_graph_col * step_size;
    double graph_y = std::sin(graph_x);

    // map this y to num_rows - 3 (for the display area on the bottom) / 2 ) + graph_y
    int x_axis_origin = (rows - 3 ) / 2;

    int row_y =  -(int)round( x_axis_origin * graph_y) + x_axis_origin;

    mvprintw(row_y, cur_graph_col, "%c", plot_symbol);

    refresh();

    return cur_graph_col + 1 ;
}

int main() {
    // Sets the global C++ locale to the user-configured system environment locale ("")
    // and synchronizes the underlying C library locale as well.
    std::locale::global(std::locale(""));

    // To ensure pre-existing standard I/O streams yield optimal results with wide characters:
    std::wcout.imbue(std::locale());
    std::wcin.imbue(std::locale());

    // ncurses init
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // CRITICAL: This allows ncurses to capture special keys like KEY_MOUSE
    mouseinterval(0);
    timeout(10);  // getch() will timeout after 10 ms so the animation can proceed


    int prows = LINES, pcols = COLS; // previous rows & columns, initialized with initial window size
    int rows, cols;

    display_bottom_info(prows, pcols);

    int cur_graph_x = 0; // for sign wave graph.
    // step size is the value we should use as the next xvalue based on the width of the screen
    // full width (cols) == 2pi.
    // half width == pi
    // etc.
    // 1 col = 2pi / full width.

    int c;
    while (  ( c = getch()) != 'q' ) {
        getmaxyx(stdscr,rows,cols);  // get current window rows, columns
        if ( c != ERR) {
            switch (c) {

                //don't care
                break;
                default:
                display_bottom_info(prows, pcols);
                mvprintw(rows - 1, 0, "Character pressed is = %3d,  '%c'", c, c);
                // must clear graph area
                break;
            }
        }
        // save last rows, cols amounts
        prows = rows;
        pcols = cols;
        // move focus/cursor to data entry spot rows-3,0
        mvprintw(prows -3, 0, "> ");

        //next iteration of sign wave printout
        cur_graph_x =  plot_graph(cur_graph_x);
    }



    endwin();
    return 0;
}
