//  ascii_3D_dung_crawl.c
//
//  Created by Rob Ross on 10/23/26.
//
//  Copyright (c) 2026.  All rights reserved.

//

#include "ascii_3D_dung_crawl.h"

#include <ncurses.h>

typedef struct {
    int start_x, start_y;
    int end_x, end_y;
} WallSegment;

// Pre-defined static screen regions for a 40x20 viewport
WallSegment C1_Wall_Face = { 2,  1, 38, 19 }; // Large centered box
WallSegment C2_Wall_Face = { 8,  4, 32, 16 }; // Medium centered box
WallSegment C3_Wall_Face = { 14, 7, 26, 13 }; // Small centered box

void draw_wall_face(WallSegment w) {
    // ncurses box-drawing macro to paint rectangles
    for(int x = w.start_x; x <= w.end_x; x++) {
        mvaddch(w.start_y, x, ACS_HLINE);
        mvaddch(w.end_y, x, ACS_HLINE);
    }
    for(int y = w.start_y; y <= w.end_y; y++) {
        mvaddch(y, w.start_x, ACS_VLINE);
        mvaddch(y, w.end_x, ACS_VLINE);
    }
    // Add corners using ACS_ULCORNER, ACS_URCORNER, etc.
}
 
