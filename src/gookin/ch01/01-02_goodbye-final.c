//  01-02_goodbye-final.c
//
//  Created by Rob Ross on 10/21/26.
//
//  Copyright (c) 2026.  All rights reserved.

//

#include <ncurses.h>

 int main() {
     initscr();
     addstr("Goodbye, cruel world!");
     refresh();
     int c;
     do {
         c = getch();  // does a wrefresh(stdscrn) before waiting for input
     } while (c == KEY_RESIZE || c == ERR);

     endwin();
     return(0);
 }
