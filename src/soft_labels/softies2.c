//  softies2.c
//
//  Created by Rob Ross on 11/4/26.
//

#define _XOPEN_SOURCE_EXTENDED 1  // Required for wide-character ncurses
#include <ncurses.h>
#include <locale.h>

#define LEFT 0
#define CENTER 1
#define RIGHT 2

#define LMAX 12

int main() {
    setlocale(LC_ALL, "");

    const char label_text[LMAX][6] = {
          "Help", "File", "Edit", "Fmt", "Find", "Block", "Ins", "Del", "View", "Switch", "Win", "Help"
    };

    slk_init(3);

    initscr();

    for (int label = 0; label < LMAX; ++label) {
        slk_set( label+1, label_text[label], CENTER);
    }

    slk_restore();

    // slk_attrset(A_ALTCHARSET );

    slk_refresh();

    getch();

    endwin();

    return 0;
}
