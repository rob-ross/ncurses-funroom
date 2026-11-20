//  radio_button.c
//
//  Created by Rob Ross on 11/17/26.
//
//  Copyright (c) 2026.  All rights reserved.

#include "radio_button.h"

#include <wchar.h>
#include <wctype.h>
#include "ncutils.h"


// 25C9   selected : ◉
// 25CB  unselected: ○


// radio item
//  state : label, selected, id, enabled, focused
//  view  : group window, pos (relative y, x)

// radio group:
//  state : radio_items[], selected index, orientation V, H, enabled, focused
//  view  : group window, mediator (unselects other buttons when one is selected)

// check box item
//  state : label, selected, id, enabled

typedef enum {
    ORIENT_HORIZONTAL, ORIENT_VERTICAL
} OrientationType;

typedef struct {
    int id;
    bool enabled;
    bool focused;

    // to support 32 bit Unicode codepoints on all platforms we store each char in 32 bits.
    // On Windows we can convert these to utf-16 encodings.
    // On platforms where wchar_t is 32 bits we can easily convert 1:1.
    const uint32_t item_label[];

} RadioItemState;

typedef struct {
    int id;
    bool enabled;
    bool focused;

    int   item_count;
    const uint32_t group_label;
    RadioItemState radio_items;

} RadioItemGroup;



constexpr wchar_t CHECKBOX_CHECK_VIZ[4] =  { L'[', L' ', L']', L'\0' };
constexpr int CHECKBOX_CHECK_VIZ_LEN = sizeof(CHECKBOX_CHECK_VIZ) / sizeof(wchar_t);

WINDOW * create_checkbox_win(int y, int x, CheckBoxItem *state) {
    // horizontal space needed : [ ] = 3 + length of label
    int item_width = (int)wcslen(state->item_label) + CHECKBOX_CHECK_VIZ_LEN;
    WINDOW * outer = newwin(3, item_width + 2, y, x);
    WINDOW * sub = derwin(outer, 1, item_width, 1, 1);
    box(outer, 0, 0);

    logfile_printf("create_checkbox_win: item_width: %d, outer:%p, sub:%p\n", item_width, outer, sub);
    return sub;
}

void draw_checkbox_win(WINDOW * cb_win, CheckBoxItem *state) {
    WINDOW * parent =  cb_win;
    WINDOW * sub = cb_win;
    if (is_subwin(cb_win)) {
        parent = wgetparent(cb_win);
    }

    if (parent) {
        // deal with border based on focus and enabled/disabled
    }

    if (sub) {
        werase(sub);
        wmove(sub, 0,0);
        wchar_t mark = state->selected ? L'X' : L' ';
        wprintw(sub, "[%lc] %ls", mark, state->item_label);
        touchwin(sub);
    }

    wnoutrefresh(parent);
}
