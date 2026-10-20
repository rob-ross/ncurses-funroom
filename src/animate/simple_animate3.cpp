//  simple_animate2.cpp
//
//  Created by Rob Ross on 10/19/26.
//
//  Copyright (c) 2026.  All rights reserved.

// based on simple_animate2.cpp, but using sub-windows for the discrete areas on screen


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

WINDOW * screen_size_w; // displays the size of the main screen
WINDOW * status_w;
WINDOW * prompt_w;

void init_ncurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // CRITICAL: This allows ncurses to capture special keys like KEY_MOUSE
}

void init_windows(int rows, int cols) {
    screen_size_w = subwin(stdscr, 1, 12, rows-1, cols - 12); // bottom-right
    status_w = subwin(stdscr, 1, 60, rows-1, 0); // bottom-left
    int remaining_width = cols - 12 - 60;
    prompt_w = subwin(stdscr, 1, remaining_width, rows-1, 60);
}

void touch_windows() {
    touchwin(stdscr);
    touchwin(prompt_w);
    touchwin(screen_size_w);
    touchwin(status_w);
}

void mvwindows(int rows, int cols) {
    // moving these doesn't seem to work. Let's try something heavier - deleting them and creating new ones
    delwin(screen_size_w);
    screen_size_w = subwin(stdscr, 1, 12, rows-1, cols - 12); // bottom-right

    // int mvwinnstr(WINDOW *win, int y, int x, char *str, int n);
    // status has at most 60 chars
    char  buf[61] = {};
    int result = mvwinnstr(status_w, 0,0, buf, 61 );
    delwin(status_w);
    status_w = subwin(stdscr, 1, 60, rows-1, 0); // bottom-left
    wmove(status_w, 0,0);
    wprintw(status_w, buf);

    int remaining_width = cols - 12 - 60; // this might be larger than before resize, causing buffer overrun below.
    // copy old prompt text to new window
    result = mvwinnstr(prompt_w, 0,0, buf, remaining_width );
    delwin(prompt_w);
    prompt_w = subwin(stdscr, 1, remaining_width, rows-1, 60);
    wmove(prompt_w, 0,0);
    wprintw(prompt_w, buf);

    touch_windows();
}

void delete_windows() {
    delwin(screen_size_w);
    delwin(status_w);
    delwin(prompt_w);
}


// display bottom center of screen
void display_prompt(const char * prompt) {
    wmove(prompt_w, 0, 0);
    int result = result = wclrtoeol(prompt_w);
    wprintw(prompt_w, prompt);
    // wnoutrefresh(prompt_w);

    wrefresh(prompt_w);
}

// display the current number of rows and columns in the bottom-left of the screen
void display_size() {
    constexpr size_t buf_size = 10;
    char buf[buf_size]{};

    int rows, cols;
    getmaxyx(stdscr,rows,cols);  // get current window rows, columns
    int result = snprintf(buf, buf_size, "%4d:%-4d", rows, cols);

    wmove(screen_size_w, 0, 0);
    result = wclrtoeol(screen_size_w);
    wprintw(screen_size_w, "|%s|", buf);
    // wnoutrefresh(screen_size_w);

    wrefresh(screen_size_w);
}

void display_status(const char * status) {
    wmove(status_w, 0, 0);
    int result = result = wclrtoeol(status_w);
    wprintw(status_w, status);
    // wnoutrefresh(status_w);

    wrefresh(status_w);
}

void update_windows() {
    // wnoutrefresh(stdscr);
    touchwin(stdscr);

    wnoutrefresh(prompt_w);
    wnoutrefresh(screen_size_w);
    wnoutrefresh(status_w);
    doupdate();
}

void redraw_windows() {
    redrawwin(prompt_w);
    redrawwin(screen_size_w);
    redrawwin(status_w);
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

    init_ncurses();

    // mouseinterval(0);
    timeout(10);  // getch() will timeout after 10 ms so the animation can proceed


    int prows = LINES; // previous rows & columns, initialized with initial window size
    int pcols = COLS;
    int rows = prows;
    int cols = pcols;

    init_windows(rows, cols);

    display_size();
    display_prompt("Press 'q' to quit.");
    display_status("");

    redraw_windows();

    int cur_graph_x = 0; // for sign wave graph.
    // step size is the value we should use as the next xvalue based on the width of the screen
    // full width (cols) == 2pi.
    // half width == pi
    // etc.
    // 1 col = 2pi / full width.

    int c;
    while (  ( c = getch()) != 'q' ) {
        getmaxyx(stdscr,rows,cols);  // get current window rows, columns
        if ( prows != rows || pcols != cols ) {
            // screen resize event
            // display_size();
            // adjust the three windows
            // move(prows, 0);
            // clrtoeol();  // erase old info
            mvwindows(rows, cols);
            display_size();
            // touchwin(stdscr);  // mark entire screen dirty

        }
        if ( c != ERR) {
            switch (c) {
                default:

                    constexpr size_t buf_size = 34;
                    char buf[buf_size]{};
                    int result = snprintf(buf, buf_size, "Character pressed is = %3d,  '%c'", c, c);
                    display_status( buf );
                    // must clear graph area on window resize
                    break;
            }
        }
        // save last rows, cols amounts
        prows = rows;
        pcols = cols;
        // // move focus/cursor to data entry spot rows-3,0
        // mvprintw(prows -3, 0, "> ");

        //next iteration of sign wave printout
        // cur_graph_x =  plot_graph(cur_graph_x);
        // refresh();
        //display_size();
        //display_prompt("Press 'q' to quit.");
        // wrefresh(stdscr);
        update_windows();


    }


    delete_windows();

    endwin();
    return 0;
}
