//  simple_color.c
//
//  Created by Rob Ross on 10/3/26.
//

// try setting the foreground/background for the entire console output

#include <ncurses.h>
#include <string.h>

int main() {
    initscr();
    start_color();
    // we're not calling cbreak() here because we just want to echo whatever is typed
    keypad(stdscr, TRUE);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    attron(COLOR_PAIR(4));

    printw("Enter text. Enter 'quit' to exit.:");
    char str_buff[1024] = {};
    while (true) {
        int res = getnstr(str_buff, 1024);
        if (res == OK ) {
            if (memcmp(str_buff, "quit", 4) == 0) {
                break;
            }
        }
    }

    endwin();

}
