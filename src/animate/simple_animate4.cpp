//  simple_animate4.cpp
//
//  Created by Rob Ross on 10/19/26.
//
//  Copyright (c) 2026.  All rights reserved.

// I abandoned trying to use windows or subwindows for now. We'll keep track of screen regions ourself.

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

struct ScreenRegion {
    int lines;
    int columns;
    int y_origin;   // origin of region with respect to stdscr
    int x_origin;
    std::string text; // text to draw for the region
    // we can add attributes later like bg/fg color, etc.


};

ScreenRegion size_region;
ScreenRegion status_region;
ScreenRegion prompt_region;

void init_ncurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // CRITICAL: This allows ncurses to capture special keys like KEY_MOUSE
}

// We can call this function whenever the screen window changes size
void init_regions(int rows, int cols) {
    size_region   = {1, 12, rows-1, cols - 12}; // bottom-right
    status_region = {1, 60, rows - 1, 0}; // bottom-left
    int remaining_width = cols - 12 - 60;
    std::string os = prompt_region.text;
    prompt_region = {1, remaining_width, rows - 1, 60, os};
}

void draw_status_region() {
    // assumes region has been cleared if required.
    move(status_region.y_origin, status_region.x_origin);
    printw(status_region.text.c_str());
}


// display prompt in bottom-left of screen
void draw_prompt_region() {
    // assumes region has been cleared if required.
    move(prompt_region.y_origin, prompt_region.x_origin);
    printw(prompt_region.text.c_str());
}

std::string get_size_string() {
    constexpr size_t buf_size = 10;
    char buf[buf_size]{};

    int rows, cols;
    getmaxyx(stdscr,rows,cols);  // get current window rows, columns
    int result;
    //allow a max of 9 col spaces for display. This allows printing of `XXXX:YYYY`, accommodating max `9999:9999`
    result = snprintf(buf, buf_size, "%4d:%-4d", rows, cols);
    return std::string{buf};
}

// display the current number of rows and columns in the bottom-right of the screen
void draw_size_region() {
    int field_width = size_region.columns;
    move( size_region.y_origin, size_region.x_origin);
    printw("|%s|", size_region.text.c_str());
}


int plot_graph(int cur_graph_col) {
    int rows, cols;
    static const char plot_symbols1[2] {'.', '.'};
    static const char plot_symbols2[2] {' ', ' '};
    static const char * symbols = plot_symbols1;
    static int ps_index = 0;

    getmaxyx(stdscr,rows,cols);  // get current window rows, columns
    if (cur_graph_col > cols - 1) {
        // reached right edge, go back to left edge
        cur_graph_col = 0;
        if ( *symbols == '.') {
            symbols = plot_symbols2;
        } else {
            symbols = plot_symbols1;
        }
    }

    double step_size = (2 * std::numbers::pi) / (double)cols;

    double graph_x = cur_graph_col * step_size;
    double graph_y = std::sin(graph_x);

    // map this y to num_rows - 3 (for the display area on the bottom) / 2 ) + graph_y
    int x_axis_origin = (rows - 3 ) / 2;

    int row_y =  -(int)round( x_axis_origin * graph_y) + x_axis_origin;

    mvprintw(row_y, cur_graph_col, "%c", symbols[ps_index]);

    ps_index = (ps_index + 1) % 2;

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

    mouseinterval(0);
    timeout(10);  // getch() will timeout after 10 ms so the animation can proceed


    int prows = LINES; // previous rows & columns, initialized with initial window size
    int pcols = COLS;
    int rows = prows;
    int cols = pcols;

    init_regions(rows, cols);
    prompt_region.text = "Press 'q' to quit.";
    size_region.text = get_size_string();
    status_region.text = "Type a key...";

    int cur_graph_x = 0; // for sign wave graph.
    // step size is the value we should use as the next xvalue based on the width of the screen
    // full width (cols) == 2pi.
    // half width == pi
    // etc.
    // 1 col = 2pi / full width.

    // todo (rob) temp
    box(stdscr, 0,0); // will this persist? no, and it takes up cells.

    int c;
    while (  ( c = getch()) != 'q' ) {
        getmaxyx(stdscr,rows,cols);  // get current window rows, columns
        if ( prows != rows || pcols != cols ) {
            // screen resize event
            // adjust the three regions
            init_regions(rows, cols);
            size_region.text = get_size_string();
            // clear graph area
            werase(stdscr); // for now clear entire window.
        }
        if ( c != ERR) {
            switch (c) {
                default:
                    constexpr size_t buf_size = 61;
                    char buf[buf_size]{};
                    int result;
                    result = snprintf(buf, buf_size, "Character pressed is = %3d,  '%c'", c, c);
                    status_region.text = buf;

                    break;
            }
        }

        wmove(stdscr, prows-2, 0); // clear from old last row to end of screen
        wclrtobot(stdscr);
        // touchwin(stdscr);  // marks whole window dirty

        // save last rows, cols amounts
        prows = rows;
        pcols = cols;

        draw_status_region();
        draw_prompt_region();
        draw_size_region();

        // do we need a refresh here?
        // surprisingly, no
        // wrefresh(stdscr);

        //next iteration of sign wave printout
        cur_graph_x =  plot_graph(cur_graph_x);

        // // move focus/cursor to data entry spot rows-3,0
        mvprintw(prows - 2, 0, "> ");

        // todo (rob) temp
        box(stdscr, 0,0);
    }


    printf("test printf\n");

    endwin();
    return 0;
}
