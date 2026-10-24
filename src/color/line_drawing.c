//  line_drawing.c
//
//  Created by Rob Ross on 10/22/26.
//
//  Copyright (c) 2026.  All rights reserved.

#define _XOPEN_SOURCE_EXTENDED 1

#include <wchar.h>
#include <ncursesw/ncurses.h>
#include <locale.h>


/*
ACS_BSSB, ACS_ULCORNER
ACS_SSBB, ACS_LLCORNER
ACS_BBSS, ACS_URCORNER
ACS_SBBS, ACS_LRCORNER
ACS_SBSS, ACS_RTEE
ACS_SSSB, ACS_LTEE
ACS_SSBS, ACS_BTEE
ACS_BSSS, ACS_TTEE
ACS_BSBS, ACS_HLINE
ACS_SBSB, ACS_VLINE
ACS_SSSS, ACS_PLUS
 */



int main()
{
    /* activate Unicode character set */
    setlocale(LC_CTYPE,"en_US.UTF-8");

    initscr();
    int cur_line = 0;
    mvaddstr(cur_line, 0, "ACS_BSSB, ACS_ULCORNER");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_ULCORNER);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_ULCORNER);

    cur_line+=2;
    mvaddstr(cur_line, 0, "ACS_SSBB, ACS_LLCORNER");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_SSBB);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_LLCORNER);

    cur_line+=2;
    mvaddstr(cur_line, 0, "ACS_BBSS, ACS_URCORNER");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_BBSS);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_URCORNER);

    cur_line+=2;
    mvaddstr(cur_line, 0, "ACS_SBBS, ACS_LRCORNER");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_SBBS);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_LRCORNER);

    cur_line+=2;
    mvaddstr(cur_line, 0, "ACS_SBSS, ACS_RTEE");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_SBSS);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_RTEE);

    cur_line+=2;
    mvaddstr(cur_line, 0, "ACS_SSSB, ACS_LTEE");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_SSSB);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_LTEE);

    cur_line+=2;
    mvaddstr(cur_line, 0, "ACS_SSBS, ACS_BTEE");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_SSBS);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_BTEE);


    cur_line+=2;
    mvaddstr(cur_line, 0, "ACS_BSSS, ACS_TTEE");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_BSSS);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_TTEE);

    cur_line+=2;
    mvaddstr(cur_line, 0, "ACS_BSBS, ACS_HLINE");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_BSBS);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_HLINE);

    cur_line+=2;
    mvaddstr(cur_line, 0, "ACS_SBSB, ACS_VLINE");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_SBSB);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_VLINE);

    cur_line+=2;
    mvaddstr(cur_line, 0, "ACS_SSSS, ACS_PLUS");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | ACS_SSSS);
    mvaddch(cur_line+1, 2, A_ALTCHARSET | ACS_PLUS);

    // NCURSES_ACS('L')
    cur_line+=2;
    mvaddstr(cur_line, 0, "NCURSES_ACS('L')");
    mvaddch(cur_line+1, 0, A_ALTCHARSET | NCURSES_ACS('L'));

    // what about an extended ACS char?
    //WACS_D_ULCORNER
    cur_line+=2;
    mvaddstr(cur_line, 0, "WACS_D_ULCORNER");
    mvadd_wch(cur_line+1, 0,  WACS_D_ULCORNER); // doesn't seem like this prints anything different than the previous

    refresh();
    // --------------------
    int row, col;
    getmaxyx(stdscr, row, col);

    move( row - 1, 0);
    refresh();


    // Loop until a genuine user keypress is detected (ignoring initial PTY resize events)
    int c;
    do {
        c = getch();
    } while (c == KEY_RESIZE || c == ERR);

    endwin();
    return 0;
}
