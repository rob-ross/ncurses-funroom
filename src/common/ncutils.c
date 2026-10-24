//  ncutils.c
//
//  Created by Rob Ross on 10/23/26.
//
//  Copyright (c) 2026.  All rights reserved.

//

#include "ncutils.h"


#include <ncurses.h>
#include <string.h>

void center(int row, char *title)
{
    int len,indent,y,width;

    /* get screen width */
    getmaxyx(stdscr,y,width);
    /* get title length */
    len = strlen(title);
    /* calculate indent */
    indent = (width - len)/2;
    /* show the string */
    mvaddstr(row,indent,title);
    refresh();
}
