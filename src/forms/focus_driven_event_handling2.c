//  focus_driven_event_handling2.c
//
//  Created by Rob Ross on 11/12/26.
//
//  Copyright (c) 2026.  All rights reserved.

// modified from focus_driven_event_handling.c to support selection and copy from input area
#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>

#include <ctype.h>
#include <string.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
    FOCUS_MENU,
    FOCUS_INPUT
} FocusArea;

static void copy_to_clipboard(const char *text, int len) {
    if (len <= 0) return;

#ifdef __APPLE__
    FILE *fp = popen("pbcopy", "w");
#else
    FILE *fp = popen("wl-copy 2>/dev/null || xclip -selection clipboard 2>/dev/null || pbcopy 2>/dev/null", "w");
#endif
    if (fp) {
        fwrite(text, 1, (size_t)len, fp);
        pclose(fp);
    }

    // OSC 52 fallback
    static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char b64[1024];
    int b64_idx = 0;
    for (int i = 0; i < len; i += 3) {
        if (b64_idx + 4 >= (int)sizeof(b64) - 1) break;
        unsigned char b0 = (unsigned char)text[i];
        unsigned char b1 = (i + 1 < len) ? (unsigned char)text[i + 1] : 0;
        unsigned char b2 = (i + 2 < len) ? (unsigned char)text[i + 2] : 0;

        b64[b64_idx++] = b64_table[b0 >> 2];
        b64[b64_idx++] = b64_table[((b0 & 0x03) << 4) | (b1 >> 4)];
        b64[b64_idx++] = (i + 1 < len) ? b64_table[((b1 & 0x0F) << 2) | (b2 >> 6)] : '=';
        b64[b64_idx++] = (i + 2 < len) ? b64_table[b2 & 0x3F] : '=';
    }
    b64[b64_idx] = '\0';
    printf("\033]52;c;%s\007", b64);
    fflush(stdout);
}

int main(void) {
    setlocale(LC_ALL, "");

    initscr();

    // Enable Bracketed Paste Mode at startup
    printf("\033[?2004h");
    fflush(stdout);

    raw();
    noecho();
    keypad(stdscr, TRUE);
    set_escdelay(10);
    mouseinterval(0);

    // Initialize ncurses mouse handling
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    // Enable Button-Motion Tracking (DECSET 1002) in standard X10 mode
    printf("\033[?1002h");
    fflush(stdout);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    WINDOW *menu_win = newwin(3, max_x, 0, 0);
    WINDOW *input_win = newwin(3, max_x, max_y - 3, 0);
    keypad(menu_win, TRUE);
    keypad(input_win, TRUE);

    FocusArea current_focus = FOCUS_INPUT;
    char buffer[256] = "Click and drag across this text to select, then press Ctrl+C to copy";
    int buf_len = (int)strlen(buffer);
    int running = 1;

    int sel_start = -1;
    int sel_end = -1;
    bool is_dragging = false;
    char status_msg[128] = "";

    while (running) {
        // 1. Redraw Menu Window
        werase(menu_win);
        box(menu_win, 0, 0);
        mvwprintw(menu_win, 1, 2, "[Menu Area] %s %s",
                  (current_focus == FOCUS_MENU) ? "(FOCUSED - 'q' to quit)" : "(Tab to focus menu)",
                  status_msg);
        wnoutrefresh(menu_win);

        // 2. Redraw Input Window with Active Selection Highlighting
        werase(input_win);
        box(input_win, 0, 0);
        mvwprintw(input_win, 1, 2, "> ");

        int has_sel = (sel_start != -1 && sel_end != -1 && sel_start != sel_end);
        int s_min = (sel_start < sel_end) ? sel_start : sel_end;
        int s_max = (sel_start < sel_end) ? sel_end : sel_start;

        for (int i = 0; i < buf_len; i++) {
            if (has_sel && i >= s_min && i < s_max) {
                wattron(input_win, A_REVERSE);
                waddch(input_win, buffer[i]);
                wattroff(input_win, A_REVERSE);
            } else {
                waddch(input_win, buffer[i]);
            }
        }

        if (current_focus == FOCUS_INPUT) {
            wmove(input_win, 1, 4 + buf_len);
        }
        wnoutrefresh(input_win);

        // Render both windows together
        doupdate();

        // 3. Wait for Input
        int ch = wgetch((current_focus == FOCUS_INPUT) ? input_win : menu_win);

        if (ch == '\t') {
            current_focus = (current_focus == FOCUS_INPUT) ? FOCUS_MENU : FOCUS_INPUT;
            snprintf(status_msg, sizeof(status_msg), "");
            continue;
        }

        if (ch == 3) { // Ctrl+C
            if (has_sel && (s_max - s_min > 0)) {
                int copy_len = s_max - s_min;
                copy_to_clipboard(buffer + s_min, copy_len);
                snprintf(status_msg, sizeof(status_msg), "[Copied %d chars to clipboard!]", copy_len);
            } else {
                snprintf(status_msg, sizeof(status_msg), "[No selection to copy]");
            }
            continue;
        }

        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                if (wenclose(input_win, event.y, event.x)) {
                    current_focus = FOCUS_INPUT;
                    int in_beg_y, in_beg_x;
                    getbegyx(input_win, in_beg_y, in_beg_x);
                    int rel_y = event.y - in_beg_y;
                    int rel_x = event.x - in_beg_x;

                    if (rel_y == 1) {
                        int char_idx = rel_x - 4;
                        if (char_idx < 0) char_idx = 0;
                        if (char_idx > buf_len) char_idx = buf_len;

                        if (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED)) {
                            sel_start = char_idx;
                            sel_end = char_idx;
                            is_dragging = true;
                            snprintf(status_msg, sizeof(status_msg), "");
                        } else if (event.bstate & BUTTON1_RELEASED) {
                            if (is_dragging) {
                                sel_end = char_idx;
                                is_dragging = false;
                                if (sel_start == sel_end) {
                                    sel_start = sel_end = -1;
                                }
                            }
                        } else {
                            // Drag motion coordinate report
                            if (is_dragging) {
                                sel_end = char_idx;
                            }
                        }
                    }
                } else if (wenclose(menu_win, event.y, event.x)) {
                    if (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED)) {
                        current_focus = FOCUS_MENU;
                        is_dragging = false;
                        sel_start = sel_end = -1;
                        snprintf(status_msg, sizeof(status_msg), "");
                    }
                } else {
                    if (event.bstate & BUTTON1_RELEASED) {
                        is_dragging = false;
                    }
                }
            }
            continue;
        }

        if (current_focus == FOCUS_INPUT) {
            if (ch == '\n' || ch == KEY_ENTER) {
                buffer[0] = '\0';
                buf_len = 0;
                sel_start = sel_end = -1;
                snprintf(status_msg, sizeof(status_msg), "[Entered text submitted]");
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (buf_len > 0) {
                    buffer[--buf_len] = '\0';
                    sel_start = sel_end = -1;
                }
            } else if (isprint(ch) && buf_len < (int)sizeof(buffer) - 1) {
                buffer[buf_len++] = (char)ch;
                buffer[buf_len] = '\0';
                sel_start = sel_end = -1;
            }
        } else if (current_focus == FOCUS_MENU) {
            if (ch == 'q' || ch == 'Q') {
                running = 0;
            }
        }
    }

    // Disable motion reporting before teardown
    printf("\033[?1002l");
    fflush(stdout);

    delwin(input_win);
    delwin(menu_win);
    endwin();
    return 0;
}

//  this text t
