//  radio_button.h
//
//  Created by Rob Ross on 11/17/26.
//
//  Copyright (c) 2026.  All rights reserved.

//

#ifndef NCURSES_FUNROOM_RADIO_BUTTON_H
#define NCURSES_FUNROOM_RADIO_BUTTON_H


#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>


typedef struct {
    int id;
    bool selected;
    bool enabled;
    bool focused;
    const wchar_t * item_label;
} CheckBoxItem;

WINDOW * create_checkbox_win(int y, int x, CheckBoxItem *state);
void draw_checkbox_win(WINDOW * cb_win, CheckBoxItem *state);

#endif //NCURSES_FUNROOM_RADIO_BUTTON_H
