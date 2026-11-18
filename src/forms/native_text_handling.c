//  native_text_handling.c
//
//  Created by Rob Ross on 11/13/26.
//
//  Copyright (c) 2026.  All rights reserved.


/**
 *  This version turns off mouse events when the text entry has the focus.
 *  This allows native text handling like selection,double-clicking, command-C copies, etc.
 *  When focus leaves the text entry, mouse events are turned back on.
 *
 *
 */
#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>
#include <wctype.h>
#include <stdio.h>
#include <unistd.h>

#include "../common/ncutils.h"

// Constants to support GNU Readline text editing shortcuts
constexpr int KEY_CTRL_A = CTRL('A'); // beginning of line, also KEY_HOME
constexpr int KEY_CTRL_E = CTRL('E'); // end of line, also KEY_END
constexpr int KEY_CTRL_B = CTRL('B'); // back 1 char, also KEY_LEFT
constexpr int KEY_CTRL_F = CTRL('F'); // forward 1 char, also KEY_RIGHT

constexpr int KEY_ALT_B = 700; // back 1 word, also option+left
constexpr int KEY_ALT_F = 701; // forward 1 word, also option+right
constexpr int KEY_CTRL_H = CTRL('H'); // delete prev character, also KEY_BACKSPACE
constexpr int KEY_CTRL_D = CTRL('D'); // delete next character, also KEY_DC
constexpr int KEY_CTRL_K = CTRL('K'); // cut to end of line ("kill")
constexpr int KEY_CTRL_U = CTRL('U'); // cut to start of line
constexpr int KEY_CTRL_W = CTRL('W'); // cut previous word
constexpr int KEY_ALT_W   = 702; // copy previous word
constexpr int KEY_CTRL_Y = CTRL('Y'); // paste/yank

constexpr int KEY_CTRL_C = CTRL('C'); // Copy current selection
constexpr int KEY_CTRL_X = CTRL('X'); // Cut current selection
constexpr int KEY_CTRL_V = CTRL('V'); // paste from app clipboard
constexpr int KEY_ALT_A  = 703; // select all

constexpr int KEY_PASTE_START = 0x703;
constexpr int KEY_PASTE_END   = 0x704;
constexpr int KEY_SHIFT_ALT_B = 0x705;
constexpr int KEY_SHIFT_ALT_F = 0x706;

// history navigation uses up/down arrow

typedef enum {
    FOCUS_MENU,
    FOCUS_INPUT
} FocusArea;

constexpr int TEXT_BUFFER_CAPACITY = 256;


// Model for a single-line text input window.
// capacity is max bytes the text model holds. assume utf-8
// size is the current number of bytes in the  buffer
// pos is the insertion point of the model. This is represented in the view by the screen cursor location
//  new text is inserted just before this point.
//  pos == 0: insert before buffer[0].
//  0 < pos < size: insert before buffer[pos].
// pos == size: insert after the last stored character.
//
// selection is -1,-1 if there is no text selection in the model, otherwise it is the half-open start,end range of the buffer
// scroll_offset is the buffer position of the leftmost buffer character displayed in the viewport.
// When the cursor is a vertical line, the position of the cursor indicates the cell just before where the next
// insertion will take place. If the cursor is a block, think of the left vertical edge of the block as the vertical
// cursor indicator. Text will be added after this position. It will appear that the char in the block is replaced and
// all text from that position is shifted right by one position. But really text is being added at that pos and the
// rest of the text right shifted by one position.

typedef struct {
    int capacity;
    int size;
    int pos; // range [0, size]
    HalfOpenRange selection; // populated when text is selected
    int selection_anchor;
    int scroll_offset; // First character visible in viewport
    wchar_t buffer[TEXT_BUFFER_CAPACITY];
    wchar_t kill_ring[TEXT_BUFFER_CAPACITY]; // App-level cut/copy buffer
    int kill_size;
} TextInputState;

static void set_mouse_enabled(bool enabled) {
    if (enabled) {
        // 1. Enable ncurses mouse parsing
        mousemask(ALL_MOUSE_EVENTS, nullptr);
        mouseinterval(0);
        // 2. Instruct terminal emulator to capture clicks
        printf("\033[?1000h");
        fflush(stdout);
    } else {
        // 1. Disable ncurses mouse parsing
        mousemask(0, nullptr);
        // 2. Instruct terminal emulator to restore native selection
        printf("\033[?1002l\033[?1000l");
        fflush(stdout);
    }
}

void enable_bracketed_paste(WINDOW *win) {
    // Tell the terminal emulator to bracket pastes with \033[200~ and \033[201~
    printf("\033[?2004h");
    fflush(stdout);

    // Map the terminal escape markers to ncurses key codes
    define_key("\033[200~", KEY_PASTE_START);
    define_key("\033[201~", KEY_PASTE_END);
}

void disable_bracketed_paste() {
    // Restore default terminal behavior on exit
    printf("\033[?2004l");
    fflush(stdout);
}

// Consumes and discards any pending characters arriving in rapid succession after an ESC
void drain_escape_sequence(WINDOW *win) {
    // 1. Capture the existing timeout configuration
    int old_delay = wgetdelay(win);

    // 2. Switch to non-blocking mode to drain in-flight characters
    wtimeout(win, 0);
    wint_t discard;
    while (wget_wch(win, &discard) != ERR) {
        // Discard trailing escape sequence bytes
    }

    // 3. Restore the original timeout configuration
    wtimeout(win, old_delay);
}

void reset_selection_state(TextInputState * state) {
    state->selection.start = -1;
    state->selection.end   = -1;
    state->selection_anchor = -1;
}

static void update_selection(TextInputState *state, int new_pos) {
    if (state->selection_anchor < 0) {
        state->selection_anchor = state->pos;
    }

    state->pos = new_pos;

    state->selection.start = state->selection_anchor < state->pos
        ? state->selection_anchor
        : state->pos;
    state->selection.end = state->selection_anchor > state->pos
        ? state->selection_anchor
        : state->pos;

    if (state->selection.start == state->selection.end) {
        reset_selection_state(state);
    }
}

// Copy current selection to internal kill_ring buffer and System clipboard
int copy_selection(TextInputState *state) {
    if (state->selection.start == - 1) return 0;

    int copy_len = state->selection.end - state->selection.start;
    if (copy_len > 0) {
        wmemcpy(state->kill_ring, &state->buffer[state->selection.start], copy_len);
        state->kill_ring[copy_len] = L'\0';
        state->kill_size = copy_len;
        // Synchronize with OS clipboard
        copy_to_system_clipboard(state->kill_ring, copy_len);
    }
    return copy_len;
}

// remove the current selection from the state buffer and move the remaining characters up
int delete_selection(TextInputState *state) {
    if (state->selection.start == - 1) return 0;
    int cut_len = state->selection.end - state->selection.start;
    if (cut_len > 0) {
        wmemmove(&state->buffer[state->selection.start],
                 &state->buffer[state->selection.end],
                 state->size - state->selection.start + 1);
        state->size -= cut_len;
    }
    return cut_len;
}

void text_input_controller(WINDOW *input_win, TextInputState *state, int char_type, wint_t ch) {
    switch (ch) {
            // jump to beginning of line
        case KEY_CTRL_A:
        case KEY_HOME: {
            state->pos = 0;
            reset_selection_state(state);
            break;
        }

            // jump to end of line
        case KEY_CTRL_E:
        case KEY_END: {
            state->pos =  state->size;
            reset_selection_state(state);
            break;
        }

            // back one char
        case KEY_CTRL_B:
        case KEY_LEFT: {
            if ( state->pos > 0) state->pos--;
            reset_selection_state(state);
            break;
        }

            // Shift-Left arrow : add the previous character to current selection
        case KEY_SLEFT: {
            logfile_printf("\nGot KEY_SLEFT. pos:%d, sel: [%d, %d)\n", state->pos, state->selection.start, state->selection.end);

            if (state->pos > 0) {
                update_selection(state, state->pos - 1);
            }

            // if ( state->pos > 0 ) {
            //     if (state->selection.end == -1 ) {
            //         // start of new selection
            //         state->selection.end = state->pos;
            //         state->selection.start = --state->pos;
            //     }  else if (state->pos > state->selection.start) {
            //         //shrinking an existing selection
            //         state->selection.end = --state->pos;
            //     }else {
            //         // expanding an existing selection
            //         state->selection.start = --state->pos;
            //     }
            // }

            logfile_printf("\nAfter KEY_SLEFT. pos:%d, sel: [%d, %d)\n", state->pos, state->selection.start, state->selection.end);

            break;
        }

            // forward one character
        case KEY_CTRL_F:
        case KEY_RIGHT: {
            if ( state->pos < state->size ) state->pos++;
            reset_selection_state(state);
            break;
        }

            // Shift-Right arrow: add next character to current selection
        case KEY_SRIGHT: {
            logfile_printf("\nGot KEY_SRIGHT. pos:%d, sel: [%d, %d)\n", state->pos, state->selection.start, state->selection.end);

            if (state->pos < state->size ) {
                update_selection(state, state->pos + 1);
            }

            // if (state->pos < state->size) {
            //     if (state->selection.end == -1 ) {
            //         // start of new selection
            //         state->selection.start = state->pos;
            //         state->selection.end = ++state->pos;
            //     } else if (state->pos < state->selection.end ) {
            //         //shrinking an existing selection
            //         state->selection.start = ++state->pos;
            //     } else {
            //         // expanding an existing selection
            //         state->selection.end = ++state->pos;
            //     }
            // }


            logfile_printf("\nAfter KEY_SRIGHT. pos:%d, sel: [%d, %d)\n", state->pos, state->selection.start, state->selection.end);
            break;
        }

            // back one word
        case KEY_ALT_B: {
            reset_selection_state(state);
            if ( state->pos == 0) {
                break; // Already at the start; no state. transition needed
            }

            // Begin inspection strictly to the left of the cursor
            int i = state->pos - 1;
            // 1. Skip whitespace immediately to the left
            while (i >= 0 && iswspace(state->buffer[i])) {
                i--;
            }

            // 2. Consume word characters to find the word boundary
            while (i >= 0 && !iswspace(state->buffer[i])) {
                i--;
            }

            // 3. Position the cursor at the first character of the word
            state->pos = i + 1;
            break;
        }

            // add the previous word to current selection
        case KEY_SHIFT_ALT_B: {
            if (state->pos == 0) {
                break;
            }

            // int anchor = state->pos;
            // if (state->selection.end != -1) {
            //     anchor = state->pos == state->selection.start
            //         ? state->selection.end
            //         : state->selection.start;
            // }

            // Begin inspection strictly to the left of the cursor
            int i = state->pos - 1;
            // 1. Skip whitespace immediately to the left
            while (i >= 0 && iswspace(state->buffer[i])) {
                i--;
            }

            // 2. Consume word characters to find the word boundary
            while (i >= 0 && !iswspace(state->buffer[i])) {
                i--;
            }
            i += 1;

            update_selection(state, i);


            // state->pos = i + 1;
            // state->selection.start = anchor < state->pos ? anchor : state->pos;
            // state->selection.end = anchor > state->pos ? anchor : state->pos;
            // if (state->selection.start == state->selection.end) {
            //     reset_selection_state(state);
            // }

            break;
        }


            // forward one word
        case KEY_ALT_F: {
            reset_selection_state(state);
            // option-right arrow seems to already be mapped to option-F

            if (state->size == 0 || state->pos == state->size ) {
                break; // no text or already at end of line
            }

            // Begin inspection strictly to the right of the cursor
            int i = state->pos + 1;

            // 1. Skip whitespace immediately to the right
            while ( i < state->size && iswspace(state->buffer[i])) {
                i++;
            }

            // 2. Consume word characters to find the word boundary
            while ( i < state->size && !iswspace(state->buffer[i])) {
                i++;
            }

            // 3. Position the cursor at the last character of the word
            state->pos = i;
            break;

        }

            // add the next word to current selection
        case KEY_SHIFT_ALT_F: {
            if (state->pos == state->size) {
                break;
            }

            // int anchor = state->pos;
            // if (state->selection.end != -1) {
            //     anchor = state->pos == state->selection.start
            //         ? state->selection.end
            //         : state->selection.start;
            // }

            // Begin inspection strictly to the right of the cursor
            int i = state->pos + 1;

            // 1. Skip whitespace immediately to the right
            while ( i < state->size && iswspace(state->buffer[i])) {
                i++;
            }

            // 2. Consume word characters to find the word boundary
            while ( i < state->size && !iswspace(state->buffer[i])) {
                i++;
            }

            // 3. Position the cursor at the last character of the word
            // state->pos = i;


            update_selection(state, i);

            // state->selection.start = anchor < state->pos ? anchor : state->pos;
            // state->selection.end = anchor > state->pos ? anchor : state->pos;
            // if (state->selection.start == state->selection.end) {
            //     reset_selection_state(state);
            // }

            break;
        }


            // delete a char
        case KEY_BACKSPACE:
        case KEY_DC:
        case 127:
        case '\b': {
            // todo delete doesn't normally add the text to the copy/kill buffer. But, to support "undo"
            //  we would need to add the deleted text to an undo buffer. And add some intelligence that
            //  multiple single deletes at the same cursor location in a certain window of time should be merged, etc.

            if (state->selection.start != -1){
                delete_selection(state);
                state->pos = state->selection.start;
            }
            else if (state->pos > 0) {
                wmemmove(
                     &state->buffer[state->pos - 1],
                     &state->buffer[state->pos],
                     state->size - state->pos + 1
                );
                state->pos--;
                state->size--;
            }
            reset_selection_state(state);
            break;
        }
            // Cut to End of Line (Ctrl-K / Kill)
        case KEY_CTRL_K: {
            int cut_len = state->size - state->pos;
            if (cut_len > 0) {
                // Save to internal kill buffer
                wmemcpy(state->kill_ring, &state->buffer[state->pos], cut_len);
                state->kill_ring[cut_len] = L'\0';
                state->kill_size = cut_len;

                // Synchronize with OS clipboard
                copy_to_system_clipboard(state->kill_ring, cut_len);

                // Truncate buffer
                state->buffer[state->pos] = L'\0';
                state->size = state->pos;
            }
            reset_selection_state(state);
            break;
        }

            // Cut to Start of Line (Ctrl-U)
        case KEY_CTRL_U: {
            int cut_len = state->pos;
            if (cut_len > 0) {
                wmemcpy(state->kill_ring, state->buffer, cut_len);
                state->kill_ring[cut_len] = L'\0';
                state->kill_size = cut_len;

                copy_to_system_clipboard(state->kill_ring, cut_len);

                wmemmove(state->buffer,
                         &state->buffer[state->pos],
                         state->size - state->pos + 1);

                state->size -= cut_len;
                state->pos = 0;
            }
            reset_selection_state(state);
            break;
        }



            // Cut Previous Word (Ctrl-W)
        case KEY_CTRL_W: {
            if (state->pos == 0) break;

            int i = state->pos - 1;
            while (i >= 0 && iswspace(state->buffer[i])) i--;
            while (i >= 0 && !iswspace(state->buffer[i])) i--;
            int word_start = i + 1;
            int cut_len = state->pos - word_start;

            if (cut_len > 0) {
                wmemcpy(state->kill_ring, &state->buffer[word_start], cut_len);
                state->kill_ring[cut_len] = L'\0';
                state->kill_size = cut_len;

                copy_to_system_clipboard(state->kill_ring, cut_len);

                wmemmove(&state->buffer[word_start],
                         &state->buffer[state->pos],
                         state->size - state->pos + 1);

                state->size -= cut_len;
                state->pos = word_start;
            }
            reset_selection_state(state);
            break;
        }


            // cut current selection
        case KEY_CTRL_X: {
            if (state->selection.start == -1 ) break;

            int sel_len = state->selection.end - state->selection.start;
            logfile_printf("KEY_CTRL_X: sel start:%d, end:%d, sel_len: %d, '%.*ls'\n",
                state->selection.start, state->selection.end, sel_len, sel_len, &state->buffer[state->selection.start]);

            int cut_len = state->selection.end - state->selection.start;
            if (cut_len > 0) {
                copy_selection(state);
                delete_selection(state);
                state->pos = state->selection.start;
            }
            reset_selection_state(state);
            break;
        }


             // Copy previous Word (Alt-W)
        case KEY_ALT_W: {
            // todo (rob) should this cause the previous word to become the selected text?
            // Determine the boundary to copy (e.g., previous word or marked range)
            if (state->pos == 0) break;

            int i = state->pos - 1;
            while (i >= 0 && iswspace(state->buffer[i])) i--;
            while (i >= 0 && !iswspace(state->buffer[i])) i--;
            int word_start = i + 1;
            int copy_len = state->pos - word_start;

            if (copy_len > 0) {
                // 1. Copy text into the internal kill ring
                wmemcpy(state->kill_ring, &state->buffer[word_start], copy_len);
                state->kill_ring[copy_len] = L'\0';
                state->kill_size = copy_len;

                // 2. Synchronize with OS clipboard
                copy_to_system_clipboard(state->kill_ring, copy_len);

                // 3. DO NOT delete text from state->buffer (buffer and pos stay unchanged)
            }
            break;
        }

            // copy current selection
        case KEY_CTRL_C: {
            if (state->selection.start == -1 ) break;

            int copy_len = state->selection.end - state->selection.start;
            copy_selection(state);

            // todo animate copy action by blinking selection

            break;
        }

            // select all
        case KEY_ALT_A: {
            state->selection.start = 0;
            state->selection.end = state->size;
            state->selection_anchor = 0;
            state->pos = state->size;
            break;
        }


            // Paste / Yank (Ctrl-Y)
        case KEY_CTRL_Y:
        case KEY_CTRL_V: {
            wchar_t paste_buf[TEXT_BUFFER_CAPACITY];
            size_t paste_len = paste_from_system_clipboard(paste_buf, TEXT_BUFFER_CAPACITY, true);

            if (paste_len == 0 && state->kill_size > 0) {
                wmemcpy(paste_buf, state->kill_ring, state->kill_size);
                paste_len = state->kill_size;
                paste_buf[paste_len] = L'\0';
            }

            if (paste_len > 0) {
                if (state->selection.start != -1 ) {
                    // we're replacing the selection with the pasted text
                    delete_selection(state);
                    state->pos = state->selection.start;
                    reset_selection_state(state);
                }

                // Clamp to remaining capacity
                if (state->size + (int)paste_len >= state->capacity) {
                    paste_len = state->capacity - 1 - state->size;
                }

                wmemmove(&state->buffer[state->pos + paste_len],
                         &state->buffer[state->pos],
                         state->size - state->pos + 1);

                wmemcpy(&state->buffer[state->pos], paste_buf, paste_len);
                state->pos += (int)paste_len;
                state->size += (int)paste_len;
            }
            break;
        }



        default: {
            // normal text to add
            if ( char_type ==  OK && isprint(ch) && state->size < state->capacity - 1) {
                wmemmove(&state->buffer[state->pos + 1],
                         &state->buffer[state->pos],
                         state->size - state->pos + 1
                );

                state->buffer[state->pos] = ch;
                state->pos++;
                state->size++;

                // todo (rob) if there is a current selection, replace it with the new text
                reset_selection_state(state);

            } else if (char_type == KEY_CODE_YES && ch == KEY_PASTE_START) {
                // paste from OS System command-V
                // todo (rob) if there is a current selection, replace it with the new text

                wint_t paste_ch;
                int p_type;

                while ((p_type = wget_wch(input_win, &paste_ch)) != ERR) {
                    if (p_type == KEY_CODE_YES && paste_ch == KEY_PASTE_END) {
                        break; // Finished consuming bracketed paste
                    }

                    // Sanitize characters: convert newlines, carriage returns, and control chars to spaces
                    if (paste_ch == L'\n' || paste_ch == L'\r' || iswcntrl(paste_ch)) {
                        paste_ch = L' ';
                    }

                    // Insert into buffer if space permits
                    if (state->size < state->capacity - 1) {
                        wmemmove(&state->buffer[state->pos + 1],
                                 &state->buffer[state->pos],
                                 state->size - state->pos + 1);

                        state->buffer[state->pos] = (wchar_t)paste_ch;
                        state->pos++;
                        state->size++;
                    }
                }

            } else {
                // we don't accept this char
                if (char_type == OK ) {
                    if (ch == '\t' ) {
                        // if it's a tab, let the main loop handle this.
                        unget_wch(ch);
                    }
                }

            }
        }
    }
}

void update_text_input_scroll_offset(TextInputState *state, int visible_width) {
    if (visible_width <= 0) return;

    // Scroll left if cursor moves before the visible window
    if (state->pos < state->scroll_offset) {
        state->scroll_offset = state->pos;
    }
    // Scroll right if cursor moves beyond the visible window
    else if (state->pos >= state->scroll_offset + visible_width) {
        state->scroll_offset = state->pos - visible_width + 1;
    }

    // Guard against negative offsets or overshooting
    if (state->scroll_offset < 0) {
        state->scroll_offset = 0;
    }
}

void draw_input_window(WINDOW * input_win, TextInputState *state, bool has_focus) {
    int max_y, max_x;
    getmaxyx(input_win, max_y, max_x);

    werase(input_win);
    box(input_win, 0, 0);

    int visible_width = max_x - 5; // left/right borders, ' > ' == 5
    update_text_input_scroll_offset( state, visible_width);

    // Render only the slice of text fitting inside the viewport
    mvwprintw(input_win, 1, 2, "> %.*ls", visible_width,  &state->buffer[ state->scroll_offset]);


    if ( has_focus ) {
        curs_set(1);
        //apply highlight to the selection, if any
        int sel_start = state->selection.start - state->scroll_offset;
        int sel_size = state->selection.end - state->selection.start;

        if (sel_size > 0 && sel_start < visible_width && sel_start + sel_size > 0) {
            if (sel_start < 0) {
                sel_size += sel_start;
                sel_start = 0;
            }

            if (sel_start + sel_size > visible_width) {
                sel_size = visible_width - sel_start;
            }
            attr_t attrs;
            short  pair;
            wattr_get( input_win, &attrs, &pair, nullptr ); // get existing
            mvwchgat(input_win, 1, 4 + sel_start, sel_size,
                     A_REVERSE, pair, nullptr);
        }

    } else {
        curs_set(0); // inactive, don't draw cursor
    }

    // Position cursor relative to screen coordinates
    int scrolled_pos = 4 + (state->pos - state->scroll_offset);
    // temp color - trying to set properties for the actual cursor doesn't work
    init_pair(8, COLOR_RED, COLOR_BLACK);
    wmove(input_win, 1, scrolled_pos );
    // wchgat(input_win, 1, A_BLINK, 8, nullptr);

    wnoutrefresh(input_win);

    logfile_printf("win:y:%d, x:%d, state.pos:%d, state.scroll_offset:%d, scrolled pos:%d, "
                   "sel x:%d, y:%d, a:%d \n",
        getcury(input_win), getcurx(input_win),
        state->pos, state->scroll_offset, scrolled_pos,
        state->selection.start, state->selection.end, state->selection_anchor);
}

// switch

int main() {
    setlocale(LC_ALL, "");
    init_log_file("forms_logfile.txt");

    initscr();
    // cbreak();
    raw(); // we need to use ctrl-C for copy, ctrl-X for cut, etc.
    noecho();
    keypad(stdscr, TRUE);
    set_escdelay(25);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    WINDOW *menu_win = newwin(3, max_x, 0, 0);
    WINDOW *input_win = newwin(3, max_x, max_y - 3, 0);
    keypad(menu_win, TRUE);
    keypad(input_win, TRUE);

    FocusArea current_focus = FOCUS_MENU;
    set_mouse_enabled(true); // Start in navigation mode with mouse enabled


    define_key("\033b", KEY_ALT_B);
    define_key("\033f", KEY_ALT_F);
    define_key("\033w", KEY_ALT_W);
    define_key("\033a", KEY_ALT_A);
    define_key("\033B", KEY_SHIFT_ALT_B);
    define_key("\033F", KEY_SHIFT_ALT_F);
    // xterm-style encoding for Shift+Option+Left Arrow.
    define_key("\033[1;4D", KEY_SHIFT_ALT_B);
    // xterm-style encoding for Shift+Option+Right Arrow.
    define_key("\033[1;4C", KEY_SHIFT_ALT_F);
    // Some terminal configurations encode Shift+Option as modifier 3.
    define_key("\033[1;3D", KEY_SHIFT_ALT_B);
    define_key("\033[1;3C", KEY_SHIFT_ALT_F);

    enable_bracketed_paste(input_win);


    const char * msg = "Type here... Click Tab/Enter to switch focus.";
    int msg_len = (int) strlen(msg);

    TextInputState input_state = {
        .capacity = TEXT_BUFFER_CAPACITY,
        .size = (int)strlen(msg),
        .pos = msg_len,
        .scroll_offset = 0,
        .selection = {.start=-1, .end=-1 },
        .selection_anchor = -1,
        .buffer =  L"Type here... Click Tab/Enter to switch focus.",
    };


    int running = 1;
    char status_msg[128] = "";

    while (running) {
        // 1. Redraw Menu Window
        werase(menu_win);
        box(menu_win, 0, 0);

        mvwprintw(menu_win, 1, 2, "[Menu Area] %s %s",
                  (current_focus == FOCUS_MENU)
                      ? "(FOCUSED - Mouse Active: Click input box or press Tab/q)"
                      : "(Tab to return focus here)",
                  status_msg);
        wnoutrefresh(menu_win);

        // 2. Redraw Input Window
        draw_input_window(input_win, &input_state, current_focus == FOCUS_INPUT);
        //
        // werase(input_win);
        // box(input_win, 0, 0);
        //
        // int visible_width = max_x - 5; // left/right borders, ' > ' = 5
        // update_text_input_scroll_offset(&input_state, visible_width);
        //
        // // Render only the slice of text fitting inside the viewport
        // mvwprintw(input_win, 1, 2, "> %.*ls", visible_width, &input_state.buffer[input_state.scroll_offset]);
        //
        // if (current_focus == FOCUS_INPUT) {
        //     // Position cursor relative to screen coordinates
        //     wmove(input_win, 1, 4 + (input_state.pos - input_state.scroll_offset));
        // }
        // wnoutrefresh(input_win);

        doupdate();

        // 3. Process Input
        wint_t ch;
        int char_type = wget_wch( current_focus == FOCUS_INPUT ? input_win : menu_win, &ch);

        // Global Focus Transition: Tab
        if (ch == '\t' ) {
            if (current_focus == FOCUS_INPUT) {
                current_focus = FOCUS_MENU;
                set_mouse_enabled(true); // Turn mouse ON for menu navigation
                snprintf(status_msg, sizeof(status_msg), "[Focus: Menu (Mouse ON)]");
            } else {
                current_focus = FOCUS_INPUT;
                set_mouse_enabled(false); // Turn mouse OFF for native text selection
                snprintf(status_msg, sizeof(status_msg), "[Focus: Input (Mouse OFF / Native Selection)]");
            }
            continue;
        }

        // Mouse Handling (Only active when current_focus == FOCUS_MENU)
        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                if (wenclose(input_win, event.y, event.x)) {
                    // Clicking the input window transitions focus to it
                    current_focus = FOCUS_INPUT;
                    set_mouse_enabled(false); // Relinquish mouse to terminal for text editing
                    snprintf(status_msg, sizeof(status_msg), "[Focus: Input (Mouse OFF / Native Selection)]");
                } else if (wenclose(menu_win, event.y, event.x)) {
                    current_focus = FOCUS_MENU;
                    snprintf(status_msg, sizeof(status_msg), "[Menu Clicked]");
                }
            }
            continue;
        }

        // Focus-Specific Key Handlers
        if (current_focus == FOCUS_INPUT) {
            if (ch == '\n' || ch == KEY_ENTER) {
                // Submit input and return focus to menu
                input_state.buffer[0] = '\0';
                input_state.size = 0;
                input_state.pos = 0;
                input_state.scroll_offset = 0;
                current_focus = FOCUS_MENU;
                set_mouse_enabled(true); // Re-enable mouse upon submission
                snprintf(status_msg, sizeof(status_msg), "[Text Submitted -> Focus: Menu (Mouse ON)]");
            } else if (char_type == OK && ch == 27) {
                // Some terminals encode Shift+Option+Arrow as ESC followed by
                // the ncurses key code (for example KEY_LEFT == 0x0104), not
                // as one byte escape sequence that define_key() can match.
                int old_delay = wgetdelay(input_win);
                wtimeout(input_win, 0);

                wint_t following_ch;
                int following_type = wget_wch(input_win, &following_ch);
                wtimeout(input_win, old_delay);

                if (following_type == KEY_CODE_YES && following_ch == KEY_LEFT) {
                    text_input_controller(input_win, &input_state,
                                          KEY_CODE_YES, KEY_SHIFT_ALT_B);
                } else if (following_type == KEY_CODE_YES && following_ch == KEY_RIGHT) {
                    text_input_controller(input_win, &input_state,
                                          KEY_CODE_YES, KEY_SHIFT_ALT_F);
                } else {
                    // Unknown escape input is filtered. Do not re-inject a
                    // non-key byte, since it could become editor text.
                    drain_escape_sequence(input_win);
                    snprintf(status_msg, sizeof(status_msg), "[Unhandled Escape Sequence Filtered]");
                }
            } else  {
                text_input_controller(input_win, &input_state, char_type, ch);
            }
        } else if (current_focus == FOCUS_MENU) {
            if (ch == 'q' || ch == 'Q') {
                running = 0;
            }
        }
    }

    // Teardown: ensure mouse capture is reset before terminal restore
    set_mouse_enabled(false);
    delwin(input_win);
    delwin(menu_win);

    disable_bracketed_paste();

    endwin();
    return 0;
}
