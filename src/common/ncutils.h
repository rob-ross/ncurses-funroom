//  ncutils.h
//
//  Created by Rob Ross on 10/23/26.
//
//  Copyright (c) 2026.  All rights reserved.

// utility functions for working with ncurses

#ifndef NCURSES_FUNROOM_NCUTILS_H
#define NCURSES_FUNROOM_NCUTILS_H

#define _XOPEN_SOURCE_EXTENDED 1  // Required for wide-character ncurses
#include <ncurses.h>

// macros to translate a printable ASCII value to/from its control-modified key code
#ifndef CTRL
#define CTRL(x) ( (x) & 0x1F )
#endif

#ifndef UNCTRL
#define UNCTRL(c) ((c) + 0x40 )   // '@'
#endif

#ifndef KEY_ESC
#define KEY_ESC 033 // decimal 27, hex 0x1B, octal 033
#endif

typedef struct {
    // represents the closed interval [start,end]
    int start; // inclusive
    int end;   // inclusive
} ClosedRange;

typedef struct {
    int start; // inclusive
    int end;   // exclusive
} HalfOpenRange;

// -----------------------------------------------------------------
//      copy/paste
// -----------------------------------------------------------------

bool copy_to_system_clipboard(const wchar_t *text, size_t len);
size_t paste_from_system_clipboard(wchar_t *dest, size_t max_capacity, bool strip_ctrl_chars);


// -----------------------------------------------------------------
//      Logging
// -----------------------------------------------------------------


void init_log_file(const char * log_filename);
void write_to_logfile(const char * msg);
void logfile_printf(const char *fmt, ...);
void close_log_file();


// misc

void center(int row, char *title);

#endif //NCURSES_FUNROOM_NCUTILS_H
