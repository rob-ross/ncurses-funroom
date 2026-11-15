//  ncutils.c
//
//  Created by Rob Ross on 10/23/26.
//
//  Copyright (c) 2026.  All rights reserved.

//

#include "ncutils.h"


#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>  // Added for va_list, va_start, va_end
#include <stdlib.h>
#include <wchar.h>



void center(int row, char *title)
{
    int len,indent,y,width;

    /* get screen width */
    getmaxyx(stdscr,y,width);
    /* get title length */
    len = (int)strlen(title);
    /* calculate indent */
    indent = (width - len)/2;
    /* show the string */
    mvaddstr(row,indent,title);
    refresh();
}

// -----------------------------------------------------------------
//      copy/paste
// -----------------------------------------------------------------


/*

To use Terminal-Native OSC 52 Sequences
1. Convert the wide string slice to UTF-8.
2. Base64 encode the UTF-8 bytes.
3. Emit printf("\033]52;c;%s\007", base64_data); fflush(stdout);.

Trade-off note: While OSC 52 provides optimal write throughput without subprocess overhead,
reading from the clipboard via OSC 52 is often disabled by default in terminal emulators for security reasons.

*/

bool copy_to_system_clipboard(const wchar_t *text, const size_t len) {
    if (!text || len == 0) return false;

    // 1. Determine required multibyte buffer size (UTF-8)
    // Create a temporary null-terminated wide slice
    wchar_t *wide_slice = malloc((len + 1) * sizeof(wchar_t));
    if (!wide_slice) return false;
    wmemcpy(wide_slice, text, len);
    wide_slice[len] = L'\0';

    size_t mb_size = wcstombs(nullptr, wide_slice, 0);
    if (mb_size == (size_t)-1) {
        free(wide_slice);
        return false;
    }

    char *mb_buffer = malloc(mb_size + 1);
    if (!mb_buffer) {
        free(wide_slice);
        return false;
    }
    wcstombs(mb_buffer, wide_slice, mb_size + 1);

    // 2. Open write pipe to macOS clipboard daemon
    FILE *pipe = popen("pbcopy", "w");
    if (!pipe) {
        free(wide_slice);
        free(mb_buffer);
        return false;
    }

    fwrite(mb_buffer, sizeof(char), mb_size, pipe);
    pclose(pipe);

    free(wide_slice);
    free(mb_buffer);
    return true;
}

size_t paste_from_system_clipboard(wchar_t *dest, size_t max_capacity, bool strip_ctrl_chars) {
    if (!dest || max_capacity == 0) return 0;

    FILE *pipe = popen("pbpaste", "r");
    if (!pipe) return 0;

    // Read UTF-8 stream from pbpaste
    char mb_buffer[1024];
    size_t bytes_read = fread(mb_buffer, 1, sizeof(mb_buffer) - 1, pipe);
    pclose(pipe);

    if (bytes_read == 0) return 0;
    mb_buffer[bytes_read] = '\0';

    if (strip_ctrl_chars) {
        for (int i = 0; i < bytes_read; ++i) {
            if (iscntrl(mb_buffer[i])) {
                mb_buffer[i] = ' ';
            }
        }
    }

    // Convert UTF-8 multibyte to wchar_t
    size_t wide_len = mbstowcs(dest, mb_buffer, max_capacity - 1);
    if (wide_len == (size_t)-1) {
        dest[0] = L'\0';
        return 0;
    }

    dest[wide_len] = L'\0';
    return wide_len;
}



// -----------------------------------------------------------------
//      Logging
// -----------------------------------------------------------------

FILE *log_file = nullptr;

void init_log_file(const char * log_filename) {
    // 1. Open the file for writing ("w" overwrites, "a" appends)
    // todo (rob) error checking, reporting.
    log_file = fopen(log_filename, "w");
}

// log message to current logfile
void write_to_logfile(const char * msg) {
    if (log_file && msg) {
        fprintf(log_file, "%s\n", msg);
        fflush(log_file);
    }
}

void logfile_printf(const char *fmt, ...) {
    if (log_file && fmt) {
        va_list args;
        va_start(args, fmt);
        vfprintf(log_file, fmt, args);
        va_end(args);
        fflush(log_file);  // Ensures log entries are flushed immediately
    }
}

void close_log_file() {
    if (log_file) {
        fflush(log_file);
        fclose(log_file);
    }
}
