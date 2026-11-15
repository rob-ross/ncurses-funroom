//  keys2.c
//
//  Created by Rob Ross on 11/11/26.
//
//  Copyright (c) 2026.  All rights reserved.

#define _XOPEN_SOURCE_EXTENDED 1  // Required for wide-character ncurses
#include <ncurses.h>
#include <limits.h>
#include <locale.h>
#include <_string.h>

#include "ncutils.h"

// modified from keys.c to read in wide chars so we can support unicode characters and meta sequences


// defining a higher level semantic key that is a combination of keys (escape sequence)
// makes it easier to work with that sequence.
// below we bind this to the sequence "\033c" for option-c

constexpr int KEY_ALT_OFFSET = KEY_MAX + 100;
#define KEY_ALT(c) (KEY_ALT_OFFSET + ((c) - 'a'))
#define KEY_ALT_NAME_LITERAL(ch) "KEY_ALT_" #ch " (Option-" #ch ")"

constexpr int KEY_SHIFT_ALT_OFFSET = KEY_MAX + 200;
#define KEY_SHIFT_ALT(c) (KEY_SHIFT_ALT_OFFSET + ((c) - 'A' ))
#define KEY_SHIFT_ALT_NAME_LITERAL(ch) "KEY_SHIFT_ALT_" #ch " (Shift-Option-" #ch ")"


// you can create bindings for meta + control combinations!!! Example:
constexpr int KEY_CTRL_ALT_OFFSET = KEY_MAX + 300;
#define KEY_CTRL_ALT(c)      (KEY_CTRL_ALT_OFFSET + ((c) - 'A'))
#define KEY_CTRL_ALT_NAME_LITERAL(ch) "KEY_CTRL_ALT_" #ch " (Ctrl-Option-" #ch ")"


// X Macros for common defines

#define FOR_EACH_ALT_KEY(DO) \
    DO('a', A) DO('b', B) DO('c', C) DO('d', D) DO('e', E) \
    DO('f', F) DO('g', G) DO('h', H) DO('i', I) DO('j', J) \
    DO('k', K) DO('l', L) DO('m', M) DO('n', N) DO('o', O) \
    DO('p', P) DO('q', Q) DO('r', R) DO('s', S) DO('t', T) \
    DO('u', U) DO('v', V) DO('w', W) DO('x', X) DO('y', Y) \
    DO('z', Z)

#define FOR_EACH_SHIFT_ALT_KEY(DO) \
    DO('A', A) DO('B', B) DO('C', C) DO('D', D) DO('E', E) \
    DO('F', F) DO('G', G) DO('H', H) DO('I', I) DO('J', J ) \
    DO('K', K) DO('L', L) DO('M', M) DO('N', N) DO('O', O) \
    DO('P', P) DO('Q', Q) DO('R', R) DO('S', S) DO('T', T) \
    DO('U', U) DO('V', V) DO('W', W) DO('X', X) DO('Y', Y) \
    DO('Z', Z)\

/* when using control-key combos, avoid :
 * CTRL-H : backspace
 * CTRL-I : horiz tab
 * CTRL-J : line feed
 * CTRL-M : carriage return/enter
 */
#define FOR_EACH_CTRL_ALT_KEY(DO) \
    DO('A', A) DO('B', B) DO('C', C) DO('D', D) DO('E', E) \
    DO('F', F) DO('G', G)  \
    DO('K', K) DO('L', L)            DO('N', N) DO('O', O) \
    DO('P', P) DO('Q', Q) DO('R', R) DO('S', S) DO('T', T) \
    DO('U', U) DO('V', V) DO('W', W) DO('X', X) DO('Y', Y) \
    DO('Z', Z)\

// define the constants we can use to check against, like KEY_ALT_A for shift-option-A, etc.
#define DEFINE_KEY_ALT_CONSTANT(chr, sym) \
    constexpr int KEY_ALT_##sym = KEY_ALT(chr);

    FOR_EACH_ALT_KEY(DEFINE_KEY_ALT_CONSTANT)
#undef DEFINE_KEY_ALT_CONSTANT

#define DEFINE_KEY_SHIFT_ALT_CONSTANT(chr, sym) \
    constexpr int KEY_SHIFT_ALT_##sym = KEY_SHIFT_ALT(chr);

    FOR_EACH_SHIFT_ALT_KEY(DEFINE_KEY_SHIFT_ALT_CONSTANT)
#undef DEFINE_KEY_SHIFT_ALT_CONSTANT

#define DEFINE_KEY_CTRL_ALT_CONSTANT(chr, sym) \
    constexpr int KEY_CTRL_ALT_##sym = KEY_CTRL_ALT(chr);

    FOR_EACH_CTRL_ALT_KEY(DEFINE_KEY_CTRL_ALT_CONSTANT)
#undef DEFINE_KEY_CTRL_ALT_CONSTANT

/*
    Mnemonic shortcuts are displayed after the menu item.
    Possible values are:
    ⌥ (Option / Alt): U+2325
    ⇧ (Shift): U+21E7
    ⌃ (Control): U+2303
    The display order is Shift, Control, Option, Letter.
    At most, 2 modifier keys are used
    Examples:
    ⌃Z, ⌥Z, ⌃⌥A, ⇧⌥B
 */
typedef struct {
    int code;
    const char *name;
    char mnemonic[10]; // symbols are multibyte utf-8
} CustomKey;

/**
 *  CUSTOM_KEYS is a sparse array lookup table to map a custom KEY code to a CustomKey entry.
 *  These are used for Option/Alt modified keys.
 *  Option + letter and Shift-Option + letter, for 52 values
 *  Control + Option + letter (lowercase only, no Shift allowed), for 22 values ( H, I, J, and M are excluded)
 *
 *  Control + lowercase letter - 26 values, ^a-^z. Case is ignored, although if the Shift key is
 *  down when Control + letter is pressed, no key event is registered.
 *
 *
 *  This lookup array is populated in init_meta_keys()
 *
 */
static constexpr int CUSTOM_KEYS_SIZE = (KEY_CTRL_ALT_OFFSET + 'Z' + 1) - KEY_ALT_OFFSET;
static  CustomKey CUSTOM_KEYS[ CUSTOM_KEYS_SIZE] = {};

/*
    For future use: note that many keys can be mapped to a key code so that when keypad(true), you can
    differentiate that key from one it is normally mapped to on the keyboard. For example, with keypad(true),
    pressing the `5` key on the keypad reports that the key `u` was pressed. To distinguish that specific
    numpad key from the keyboard `u` key, you can bind it like:
    // KEY_B2 is standard ncurses for keypad center / 5
    define_key("\033Ou", KEY_B2);

    You can also define the remaining keypad sequences if needed:

    define_key("\033Op", KEY_LL);  // Keypad 0
    define_key("\033Oq", KEY_C1);  // Keypad 1 (End)
    define_key("\033Or", KEY_C2);  // Keypad 2 (Down)
    define_key("\033Os", KEY_C3);  // Keypad 3 (PgDn)
    define_key("\033Ot", KEY_B1);  // Keypad 4 (Left)
    define_key("\033Ov", KEY_B3);  // Keypad 6 (Right)
    define_key("\033Ow", KEY_A1);  // Keypad 7 (Home)
    define_key("\033Ox", KEY_A2);  // Keypad 8 (Up)
    define_key("\033Oy", KEY_A3);  // Keypad 9 (PgUp)

    interestingly, ctrl-Space is mapped to value 0. Which is the same as NULL.
    I wonder how complicated it would be to support ctrl-Space as a valid key shortcut
*/

void init_meta_keys() {
    char seq[3] = "\033A";
    int code;
    int lookup_key;
#define INIT_ALT_KEY(chr, sym) \
        seq[1] = chr; \
        code =  KEY_ALT(chr);\
        define_key(seq, code); \
        lookup_key = code - KEY_ALT_OFFSET;\
        CUSTOM_KEYS[lookup_key] = (CustomKey){ \
            .code = code, \
            .name = KEY_ALT_NAME_LITERAL(sym), \
            .mnemonic = "⌥" #sym  \
        };

        FOR_EACH_ALT_KEY(INIT_ALT_KEY)
#undef INIT_ALT_KEY

#define INIT_SHIFT_ALT_KEY(chr, sym) \
        seq[1] = chr; \
        code =  KEY_SHIFT_ALT(chr);\
        define_key(seq, code); \
        lookup_key = code - KEY_ALT_OFFSET;\
        CUSTOM_KEYS[lookup_key] = (CustomKey){ \
            .code = code, \
            .name = KEY_SHIFT_ALT_NAME_LITERAL(sym), \
            .mnemonic = "⇧⌥" #sym  \
        };

    FOR_EACH_SHIFT_ALT_KEY(INIT_SHIFT_ALT_KEY)
#undef INIT_SHIFT_ALT_KEY

#define INIT_CTRL_ALT_KEY(chr, sym) \
        seq[1] = (char)CTRL(chr); \
        code =  KEY_CTRL_ALT(chr);\
        define_key(seq, code); \
        lookup_key = code - KEY_ALT_OFFSET;\
        CUSTOM_KEYS[lookup_key] = (CustomKey){ \
            .code = code, \
            .name = KEY_CTRL_ALT_NAME_LITERAL(sym), \
            .mnemonic = "⌃⌥" #sym \
        };

        FOR_EACH_CTRL_ALT_KEY(INIT_CTRL_ALT_KEY)
#undef INIT_CTRL_ALT_KEY

    // todo temp debug
    for (int i = 0; i < CUSTOM_KEYS_SIZE; ++i) {
        CustomKey ck = CUSTOM_KEYS[i];
        logfile_printf("CUSTOM_KEYS[%d] = { .code = %d, .name = '%s', .mnemonic = '%s' }\n",
               i, ck.code, ck.name, ck.mnemonic);
    }
}




const char *resolve_keyname(int ret, wint_t ch) {
    if (ret == KEY_CODE_YES) {
        CustomKey *ck = nullptr;
        int lookup_key = ch - KEY_ALT_OFFSET;

        logfile_printf("resolve_keyname: ch=%d, key=%d \n", ch, lookup_key);
        if ( lookup_key >= 0 && lookup_key < CUSTOM_KEYS_SIZE) {
            ck = &CUSTOM_KEYS[lookup_key];
            if (ck) {
                return ck->name;
            }
        }

        // Fall back to standard ncurses key names (KEY_UP, KEY_F(1), etc.)
        const char *name = keyname((int)ch);
        return name ? name : "UNKNOWN_KEY";
    }

    // Normal Unicode / ASCII character
    return key_name(ch);
}

void print_quitting_instructions(bool raw_mode, bool keypad_mode, int row) {
    wmove(stdscr, row, 0); wclrtoeol(stdscr);
    wprintw(stdscr,"Press ^Q to quit.");
    wmove(stdscr, row+1, 0); wclrtoeol(stdscr);
    wprintw(stdscr,"Press ^K to toggle keypad mode.");
    wmove(stdscr, row+2, 0); wclrtoeol(stdscr);
    wprintw(stdscr,"Press ^R to toggle raw/cbreak mode.");
}

void print_state(bool raw_mode, bool keypad_mode, int row) {
    wmove(stdscr, row, 0);
    wclrtobot(stdscr);
    mvwprintw(stdscr, row, 0, "keypad: %s", keypad_mode ? "true" : "false");
    mvwprintw(stdscr, row + 1, 0, "raw mode: %s", raw_mode ? "raw" : "cbreak");
    wrefresh(stdscr);
}

// Consumes and discards any pending characters arriving in rapid succession after an ESC
// if out_buff is non nullptr, the discarded chars will be written to the buffer, which must
// have sufficient size. 10 elements should be good enough?
void drain_escape_sequence(WINDOW *win, wint_t * out_buff) {
    // 1. Capture the existing timeout configuration
    int old_delay = wgetdelay(win);

    // 2. Switch to non-blocking mode to drain in-flight characters
    wtimeout(win, 0);
    wint_t discard;
    int char_count = 0;
    if (out_buff) {
        out_buff[char_count++] = KEY_ESC;
    }
    while (wget_wch(win, &discard) != ERR) {
        // Discard trailing escape sequence bytes
        if (out_buff) {
            out_buff[char_count++] = discard;
        }
    }
    if (out_buff) {
        out_buff[char_count] = '\0';
    }

    // 3. Restore the original timeout configuration
    wtimeout(win, old_delay);
}

int main() {
    setlocale(LC_ALL, "");
    init_log_file("keys_log.txt");
    write_to_logfile("keys2 started.");

    initscr();
    raw();
    bool raw_mode = true;
    noecho();
    curs_set(0); // hide cursor

    bool keypad_mode = true;
    keypad(stdscr, keypad_mode);
    set_escdelay(50);

    // defining a higher level semantic key that is a combination of keys (escape sequence)
    // makes it easier to work with that sequence.
    init_meta_keys();


    werase(stdscr);
    print_quitting_instructions(raw_mode, keypad_mode, 0);
    print_state(raw_mode, keypad_mode, 15);

    wint_t esc_buffer[10] = {};

    while ( true ) {

        wint_t ch;
        int ret = wget_wch(stdscr, &ch); // blocks

        // print_quitting_instructions(raw_mode, keypad_mode, 0);
        // print_state(raw_mode, keypad_mode, 15);

        cchar_t cchar = { .attr = A_NORMAL, .chars = { ch }};

        wmove(stdscr, 5, 0);
        wclrtobot(stdscr);

        if (ret == KEY_CODE_YES) {
            /* Function key or arrow key (e.g., KEY_UP, KEY_F(1)) */
            wprintw(stdscr, "KEY_CODE_YES:\n\nkey_name: %s\nkey code: (0x%.2X, dec %d)",
                resolve_keyname(ret, ch), ch, ch);

        } else if (ret == OK) {
            if (ch == CTRL('q')) {
                break; // exit main loop, quit
            }

            // Full Unicode character (e.g., L'å' = 0x00E5, L'∂' = 0x2202, L'A' = 0x0041)
            wchar_t wchar_str[4] = { ch };
            wprintw(stdscr, "OK:\nkey pressed: ");
            if (ch == KEY_ESC) {
                wprintw(stdscr, "ESC");
            } else if (ch == 0 ) {
                wprintw(stdscr, "NULL");
            } else {
                wprintw(stdscr, "'%lc'", ch);
            }
            wprintw(stdscr, "\nkey_name: %s\nkey code: (0x%.2X, dec %d)", resolve_keyname(ret, ch), ch, ch);
            if ( ch < 0x20 ) {
                wprintw(stdscr, "\ncontrol mask: (ch & 0x1F)= 0x%.2X", CTRL(ch));
            }

        }

        if ( ch == CTRL('k')) {
            keypad_mode = !keypad_mode;
            keypad(stdscr, keypad_mode);
        } else if (ch == CTRL('r')) {
            raw_mode = !raw_mode;
            if (raw_mode) {
                nocbreak();
                raw();
            } else {
                noraw();
                cbreak();
            }
        }


        // shows how we can differentiate between a key press and one with control + keypress
        if ( ret == OK ) {
            if (ret == OK &&  ch < 0x20 ) {
                // Control key modification detected
                char base_key_upper = (char)(ch + '@');      // 1 -> 'A', 26 -> 'Z'
                char base_key_lower = (char)(ch + 'a' - 1);  // 1 -> 'a', 26 -> 'z'
                // or more simply UNCTRL(ch)
                mvwprintw(stdscr, 11, 0, "You pressed '^%c'", UNCTRL(ch));
            } else {
                mvwprintw(stdscr, 11, 0, "You pressed '%c'", ch);
            }
        }

        // shows how to check a specific key
        if ( ret == OK && ch == CTRL('z') ){
            mvwprintw(stdscr, 11, 0, "You pressed 'control-z'");
        } else if (ret == OK && ch == 'z') {
            mvwprintw(stdscr, 11, 0, "You pressed 'z'");
        }

        // using keycode constants to check for option key combinations
        if (ret == KEY_CODE_YES ) {
            if ( ch == KEY_ALT_Z) {
                mvwprintw(stdscr, 11, 0, "You pressed 'option-z'");
            } else if ( ch == KEY_SHIFT_ALT_Z) {
                mvwprintw(stdscr, 11, 0, "You pressed 'shift-option-z'");
            } else if ( ch == KEY_CTRL_ALT_Z) {
                mvwprintw(stdscr, 11, 0, "You pressed 'control-option-z'");

            }
        }

        // unhandled/unknown esc sequence. Drain the sequence and capture the chars for display
        if (ch == KEY_ESC) {
            drain_escape_sequence(stdscr, esc_buffer);
            wprintw(stdscr, "\nUnknow ESC sequence: '%ls' : ", esc_buffer);
            for (wint_t *p = esc_buffer; *p; p++) {
                wprintw(stdscr, " 0x%.4X", *p);
            }
        }

        print_state(raw_mode, keypad_mode, 15);
        wrefresh(stdscr);
    } // while ( true )

END:
    endwin();

    close_log_file();
}
