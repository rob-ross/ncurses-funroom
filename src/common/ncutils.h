//  ncutils.h
//
//  Created by Rob Ross on 10/23/26.
//
//  Copyright (c) 2026.  All rights reserved.

// utility functions for working with ncurses

#ifndef NCURSES_FUNROOM_NCUTILS_H
#define NCURSES_FUNROOM_NCUTILS_H

void center(int row, char *title);

#ifdef __STDC_ISO_10646__
// Safe to assume wchar_t values map directly to Unicode/UCS-4 code points
#else
// wchar_t may be using a local encoding (like Shift-JIS) or UTF-16
#endif


#endif //NCURSES_FUNROOM_NCUTILS_H
