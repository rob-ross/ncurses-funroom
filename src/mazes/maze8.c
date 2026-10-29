//  maze8.c
//
//  Created by Rob Ross on 10/28/26.
//
//  Copyright (c) 2026.  All rights reserved.

// modified from maze 7 to switch to a bit flag model for each cell with the 4 directions available

#define _XOPEN_SOURCE_EXTENDED 1 // Enforce POSIX and wide-character terminal extensions
#include <assert.h>
#include <wchar.h>
#include <wctype.h> // Wide character classification and conversion
#include <ncursesw/ncurses.h>



#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#define MAX_ROWS 100
#define MAX_COLS 100

// NCURSES Character cell bounds
#define VIEW_W 40
#define VIEW_H 20

// Define unique index handles for color configurations
#define COLOR_WALL 1
#define COLOR_TEXT 2

// High-Resolution Sub-Pixel Braille Canvas Map (2x4 pixels per text cell)
#define PIXEL_W (VIEW_W * 2) // 80 Pixels wide
#define PIXEL_H (VIEW_H * 4) // 80 Pixels high

typedef enum { DIR_NORTH = 0, DIR_EAST = 1, DIR_SOUTH = 2, DIR_WEST = 3 } Direction;

typedef struct {
    int x;
    int y;
} Coord;

typedef struct {
    Coord pos;
    Direction dir;
} Player;

typedef struct {
    char cells[4][3];
} ViewFrustum;

// Wireframe layer structure mapping straight to pixel space coordinates
typedef struct {

    char grid[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
} Map;

//Bit flag masks to signify presence of corresponding wall in a square.
// Leftmost bit is for West, rightmost bit is for East, North and South are middle bits.
// I chose this over a N,E,S,W encoding order because it's easier to mentally create
// the right bitmask for a square with this visual model  W  N   S  E
typedef enum : unsigned char {
    WF_WEST  = 1u << 3,
    WF_NORTH = 1u << 2,
    WF_SOUTH = 1u << 1,
    WF_EAST  = 1u,

} WallFlags;
// Models a map as a 2D array of gridsquares of equal size.
// Each gridsquare is a bit field containing the location of any of 4 walls surrounding the square

typedef struct {
    int rows;
    int cols;
    WallFlags walls[MAX_ROWS][MAX_COLS];
    // todo temp - this is wasteful of memory. We need a hashmap from row,col to a list of objects at that grid location
    wint_t    objects[MAX_ROWS][MAX_COLS];
} Map2;


typedef struct {
    int x0, y0, x1, y1;
} WallRect;

// High-res pixel footprints scaled to fit beautifully inside the 80x80 matrix canvas
WallRect depth_rects[4] = {
    {0, 0, 79, 79}, // Depth 0 (Canvas edge bounds)
    {12, 12, 67, 67}, // Depth 1 (Close wall face layer)
    {24, 24, 55, 55}, // Depth 2 (Mid-distance layer)
    {32, 32, 47, 47} // Depth 3 (Far-distance layer)
};

// Global Game State
Map game_map;
Map2 game_map2;

Player player;
int game_running = 1;
int player_won = 0;

// The Virtual High-Resolution Graphics Byte-Buffer Screen
unsigned char pixel_buffer[PIXEL_H][PIXEL_W];

// Clear the vector canvas before each rendering update pass
void clear_pixel_buffer() {
    for (int y = 0; y < PIXEL_H; y++) {
        for (int x = 0; x < PIXEL_W; x++) {
            pixel_buffer[y][x] = 0;
        }
    }
}

// Set a single sub-pixel dot on the high-res frame buffer canvas
void set_pixel(int x, int y) {
    if (x >= 0 && x < PIXEL_W && y >= 0 && y < PIXEL_H) {
        pixel_buffer[y][x] = 1;
    }
}

// Pure Integer Bresenham's Line Algorithm for perfect vectors
void draw_vector_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        set_pixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Render a front-facing 3D square frame using high-res lines
void draw_braille_front_wall(WallRect r) {
    draw_vector_line(r.x0, r.y0, r.x1, r.y0); // Top
    draw_vector_line(r.x0, r.y1, r.x1, r.y1); // Bottom
    draw_vector_line(r.x0, r.y0, r.x0, r.y1); // Left side
    draw_vector_line(r.x1, r.y0, r.x1, r.y1); // Right side
}

// Render smooth diagonal side-wall extensions
void draw_braille_side_wall(WallRect near_r, WallRect far_r, int left_side, int right_side) {
    if (left_side) {
        draw_vector_line(near_r.x0, near_r.y0, far_r.x0, far_r.y0); // Ceiling diagonal slant
        draw_vector_line(near_r.x0, near_r.y1, far_r.x0, far_r.y1); // Floor diagonal slant
    }
    if (right_side) {
        draw_vector_line(far_r.x1, far_r.y0, near_r.x1, near_r.y0); // Ceiling diagonal slant
        draw_vector_line(far_r.x1, far_r.y1, near_r.x1, near_r.y1); // Floor diagonal slant
    }
}

// Clear a rectangle face down inside the high-res pixel array buffer to handle occlusions
void clear_braille_sub_rect(WallRect r) {
    for (int y = r.y0 + 1; y < r.y1; y++) {
        for (int x = r.x0 + 1; x < r.x1; x++) {
            if (x >= 0 && x < PIXEL_W && y >= 0 && y < PIXEL_H) {
                pixel_buffer[y][x] = 0;
            }
        }
    }
}

// Process and compress the 80x80 byte array down into Unicode Braille patterns for display
void flush_buffer_to_screen(int start_x, int start_y) {
    // Binary weight maps for standard 2x4 cell dots structure
    int dot_map[4][2] = {
        {0x01, 0x08}, // Row 0: Dot 1, Dot 4
        {0x02, 0x10}, // Row 1: Dot 2, Dot 5
        {0x04, 0x20}, // Row 2: Dot 3, Dot 6
        {0x40, 0x80} // Row 3: Dot 7, Dot 8
    };

    for (int ch_y = 0; ch_y < VIEW_H; ch_y++) {
        for (int ch_x = 0; ch_x < VIEW_W; ch_x++) {
            int braille_char_offset = 0x2800; // Base index for Unicode Braille block

            // Sample the 2x4 sub-pixels inside this character cell's space bounds
            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 2; c++) {
                    int pixel_x = (ch_x * 2) + c;
                    int pixel_y = (ch_y * 4) + r;

                    if (pixel_buffer[pixel_y][pixel_x]) {
                        braille_char_offset += dot_map[r][c];
                    }
                }
            }

            // Convert to broad multi-byte string context sequence array safely
            wchar_t wstr[2] = {(wchar_t) braille_char_offset, L'\0'};
            move(start_y + ch_y, start_x + ch_x);
            addwstr(wstr);
        }
    }
}

int load_map(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;
    game_map.rows = game_map.cols = 0;
    char line[MAX_COLS + 2];
    while (fgets(line, sizeof(line), file) && game_map.rows < MAX_ROWS) {
        int c = 0;
        while (line[c] != '\n' && line[c] != '\0' && c < MAX_COLS) {
            game_map.grid[game_map.rows][c] = line[c];
            if (line[c] == 'P') {
                player.pos.x = c;
                player.pos.y = game_map.rows;
                player.dir = DIR_NORTH;
                game_map.grid[game_map.rows][c] = ' ';
            }
            c++;
        }
        if (c > game_map.cols) game_map.cols = c;
        game_map.rows++;
    }
    fclose(file);
    return 1;
}

// This version loads the data as bit fields for each map coordinate.
// Flags indicate walls in the N, E, S, or W of the grid square.
// Each character is a hex nibble 0-F. This can be expanded as needed by any printable ASCII range, line 0-Z, etc.
// to encode a bit pattern
int load_map2(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;
    game_map2.rows = game_map2.cols = 0;
    char line[MAX_COLS + 2];  // +2 to account for \n and \0 terminator
    while (fgets(line, sizeof(line), file) && game_map2.rows < MAX_ROWS) {
        int c = 0;
        while (line[c] != '\n' && line[c] != '\0' && c < MAX_COLS) {
            char grid_char =  line[c];
            // bad chars are interpreted as a solid wall square
            if (grid_char < '0' || grid_char > 'F') grid_char = 'F';
            unsigned char flags = strtol( (char[2]){ grid_char,'\0'}, nullptr, 16);
            game_map2.walls[game_map2.rows][c] = flags;

            c++;
        }
        if (c > game_map2.cols) game_map2.cols = c;
        game_map2.rows++;
    }
    // todo we're currently hardcoding the entrance, exit for this map file
    player.pos.x = player.pos.y = 0;
    player.dir = DIR_NORTH;

    fclose(file);
    return 1;

}

char get_tile_at(Coord coord) {
    if (coord.x < 0 || coord.x >= game_map.cols || coord.y < 0 || coord.y >= game_map.rows) return '#';
    return game_map.grid[coord.y][coord.x];
}

ViewFrustum update_frustum() {
    ViewFrustum f;
    int fx = 0, fy = 0, rx = 0, ry = 0;
    switch (player.dir) {
        case DIR_NORTH: fx = 0;
            fy = -1;
            rx = 1;
            ry = 0;
            break;
        case DIR_EAST: fx = 1;
            fy = 0;
            rx = 0;
            ry = 1;
            break;
        case DIR_SOUTH: fx = 0;
            fy = 1;
            rx = -1;
            ry = 0;
            break;
        case DIR_WEST: fx = -1;
            fy = 0;
            rx = 0;
            ry = -1;
            break;
    }
    for (int depth = 0; depth <= 3; depth++) {
        for (int offset = -1; offset <= 1; offset++) {
            Coord target = {player.pos.x + (fx * depth) + (rx * offset), player.pos.y + (fy * depth) + (ry * offset)};
            f.cells[depth][offset + 1] = get_tile_at(target);
        }
    }
    return f;
}

void turn_player(int rotation) { player.dir = (player.dir + rotation + 4) % 4; }

void move_player(int step) {
    int dx = 0, dy = 0;
    switch (player.dir) {
        case DIR_NORTH: dy = -step;
            break;
        case DIR_SOUTH: dy = step;
            break;
        case DIR_EAST: dx = step;
            break;
        case DIR_WEST: dx = -step;
            break;
    }
    Coord target = {player.pos.x + dx, player.pos.y + dy};
    char tile = get_tile_at(target);
    if (tile == ' ') { player.pos = target; } else if (tile == 'E') {
        player.pos = target;
        player_won = 1;
        game_running = 0;
    }
}

void render_2D_map(int left_panel_x) {

    // PANEL A: 2D Minimap Overhead with doubled horizontal characters
    for (int y = 0; y < game_map.rows; y++) {
        for (int x = 0; x < game_map.cols; x++) {
            char cell_char = game_map.grid[y][x];
            int screen_x = left_panel_x + (x * 2);
            if (cell_char == '#') {
                // Combine space characters with the background color attribute
                mvaddch(y, screen_x,     ' ' | COLOR_PAIR(COLOR_WALL));
                mvaddch(y, screen_x + 1, ' ' | COLOR_PAIR(COLOR_WALL));
            }
            else if (cell_char == 'E') {
                mvaddch(y, screen_x,     '[');
                mvaddch(y, screen_x + 1, 'E');
            }
            else {
                // Ensure paths use standard background colors
                mvaddch(y, screen_x,     ' ' | COLOR_PAIR(COLOR_TEXT));
                mvaddch(y, screen_x + 1, ' ' | COLOR_PAIR(COLOR_TEXT));
            }
        }
    }
    char avatars[] = {'^', '>', 'v', '<'};
    mvaddch(player.pos.y, left_panel_x + (player.pos.x * 2) + 1, avatars[player.dir]);
}

#define HAS_FLAGS(val, mask) (((val) & (mask)) == (mask))

constexpr wint_t JOINERS_MAP[16][2] = {
    { L' ', L'\0' },
    { L' ', L'\0' },
    { L' ', L'\0' },
    { L'┘', L'\0' }, // 2518
    { L'│', L'\0' }, // 2502
    { L'│', L'\0' }, // 2502
    { L'┐', L'\0' }, // 2510
    { L'┤', L'\0' }, // 2524
    { L'─', L'\0' }, // 2500
    { L'└', L'\0' }, // 2514
    { L'─', L'\0' }, // 2500
    { L'┴', L'\0' }, // 2534
    { L'┌', L'\0' }, // 250C
    { L'├', L'\0' }, // 251C
    { L'┬', L'\0' }, // 252C
    { L'┼', L'\0' }, // 253C
};

const wint_t * get_joiner_wstr(unsigned index) {
    assert( index <= 15 );
    return JOINERS_MAP[index];
}

const wint_t *  get_joiner_for_flags(bool north, bool west, bool left_north, bool above_west) {
    unsigned index = 0;
    index |= ( 1u * north << 3);
    index |= ( 1u * west  << 2);
    index |= ( 1u * left_north << 1);
    index |= ( 1u * above_west);
    return get_joiner_wstr(index);
}

void render_2D_map2(Map2 * gm, int left_panel_x, int left_panel_y) {
    // PANEL A: 2D Minimap Overhead with doubled horizontal terminal cells to make grid more square.
    // We now have to draw the borders around each cell, so that's 3 cells wide for left-cell-right
    // and three cells tall for top-cell-right. These may end up looking like a rectangle and not a square
    // since cell is now 3 tall, we *should* draw it 6 wide.
    int col_scale = 6; // we scale the grid square up by this amount for cols
    int row_scale = 3;
    // we'll try 6 x 3 cells

    //todo debug
    int max_rows, max_cols;
    getmaxyx(stdscr, max_rows, max_cols);

    for (int y = 0; y < gm->rows; y++) {
        for (int x = 0; x < gm->cols; x++) {
            // draw gridsquare borders
            WallFlags flags = gm->walls[y][x];
            // screen_x and _screen_y should be the scaled values of the top-left
            // coordinate of each terminal square

            int screen_x = left_panel_x + (x * (col_scale - 1))  ; // maybe -1
            int screen_y = left_panel_y + (y * (row_scale - 1))  ; // maybe -1

            //todo debug
            // mvprintw(max_rows - 1 - y - 1, x * 12, "y:%d,x:%d, ", screen_y, screen_x);
            // refresh();

            if (flags & WF_NORTH) {
                for (int i = 0; i < col_scale ; ++i) {
                    mvaddwstr(screen_y, screen_x+i, L"\u2500");
                }
            }
            if (flags & WF_WEST) {
                for (int i = 0; i < row_scale ; ++i) {
                    mvaddwstr(screen_y + i, screen_x , L"\u2502");
                }
            }

            WallFlags above_flags = (y <= 0 ) ? 0 : gm->walls[y-1][x];
            WallFlags left_flags  = (x <= 0 ) ? 0 : gm->walls[y][x-1];
            const wint_t * joiner_str = get_joiner_for_flags(
                flags & WF_NORTH, flags & WF_WEST, left_flags & WF_NORTH, above_flags & WF_WEST  );

            mvaddwstr(screen_y, screen_x, joiner_str);
            getch();

            // todo special handling for last row in map
            if ( y == gm->rows - 1 ) {
                // we have to render the south edges
                if (flags & WF_SOUTH) {
                    for (int i = 0; i < col_scale; ++i) {
                        mvaddwstr(screen_y + row_scale, screen_x+i, L"\u2500");
                    }
                }
                //joiners
                joiner_str = get_joiner_for_flags(
                0, flags & WF_EAST, flags & WF_NORTH, 0  );
                mvaddwstr(screen_y, screen_x + col_scale, joiner_str);
                printw("last row");
                getch();
            }

            // todo special handling for last column in row
            if (x == gm->cols - 1 ) {
                // last grid square in row
                // we have to render the south and east edges
                // if (flags & WF_SOUTH) {
                //     for (int i = 0; i < col_scale; ++i) {
                //         mvaddwstr(screen_y + row_scale, screen_x+i, L"\u2500");
                //     }
                // }
                if (flags & WF_EAST) {
                    for (int i = 0; i < row_scale; ++i) {
                        mvaddwstr(screen_y + i, screen_x + col_scale , L"\u2502");
                    }
                }
                //joiners
                joiner_str = get_joiner_for_flags(
                0, flags & WF_EAST, flags & WF_NORTH, 0  );
                mvaddwstr(screen_y, screen_x + col_scale, joiner_str);
            }
        }
    }

    char avatars[] = {'^', '>', 'v', '<'};
    mvaddch(left_panel_y + player.pos.y*row_scale + 1, left_panel_x + (player.pos.x * col_scale) + 2, avatars[player.dir]);

    //hard coding E for now
    mvaddch(2*(row_scale +1), 9 * col_scale + 2, 'E');
}

void render_3D_view(int right_panel_x, int viewport_y) {
        // PANEL B: Virtual Interface Framework

    ViewFrustum frustum = update_frustum();
    clear_pixel_buffer(); // Fresh slate step for vector processing buffer canvas


    mvprintw(0, right_panel_x, "--- 3D BRAILLE VECTOR ENGINE ---");

    // --- HIGH-RESOLUTION RASTERIZATION DRAW PIPELINE ---

    // LAYER 3: Far Distance (Depth 3)
    if (frustum.cells[3][1] == '#' || frustum.cells[3][1] == 'E') {
        draw_braille_front_wall(depth_rects[3]);
    }

    // LAYER 2: Mid Distance (Depth 2)
    if (frustum.cells[2][0] == '#') {
        draw_braille_side_wall(depth_rects[2], depth_rects[3], 1, 0);
        draw_vector_line(depth_rects[2].x0, depth_rects[2].y0, depth_rects[2].x0, depth_rects[2].y1);
        draw_vector_line(depth_rects[3].x0, depth_rects[3].y0, depth_rects[3].x0, depth_rects[3].y1);
    }
    if (frustum.cells[2][2] == '#') {
        draw_braille_side_wall(depth_rects[2], depth_rects[3], 0, 1);
        draw_vector_line(depth_rects[2].x1, depth_rects[2].y0, depth_rects[2].x1, depth_rects[2].y1);
        draw_vector_line(depth_rects[3].x1, depth_rects[3].y0, depth_rects[3].x1, depth_rects[3].y1);
    }
    if (frustum.cells[2][1] == '#' || frustum.cells[2][1] == 'E') {
        clear_braille_sub_rect(depth_rects[2]);
        draw_braille_front_wall(depth_rects[2]);
    }

    // LAYER 1: Close Distance (Depth 1)
    if (frustum.cells[1][0] == '#') {
        draw_braille_side_wall(depth_rects[1], depth_rects[2], 1, 0);
        draw_vector_line(depth_rects[1].x0, depth_rects[1].y0, depth_rects[1].x0, depth_rects[1].y1);
        draw_vector_line(depth_rects[2].x0, depth_rects[2].y0, depth_rects[2].x0, depth_rects[2].y1);
    }
    if (frustum.cells[1][2] == '#') {
        draw_braille_side_wall(depth_rects[1], depth_rects[2], 0, 1);
        draw_vector_line(depth_rects[1].x1, depth_rects[1].y0, depth_rects[1].x1, depth_rects[1].y1);
        draw_vector_line(depth_rects[2].x1, depth_rects[2].y0, depth_rects[2].x1, depth_rects[2].y1);
    }

    if (frustum.cells[1][1] == '#' || frustum.cells[1][1] == 'E') {
        clear_braille_sub_rect(depth_rects[1]);
        draw_braille_front_wall(depth_rects[1]);
    }
    // LAYER 0: Immediate walls flanking player coordinates
    if (frustum.cells[0][0] == '#') {
        draw_braille_side_wall(depth_rects[0], depth_rects[1], 1, 0);
        draw_vector_line(depth_rects[0].x0, depth_rects[0].y0, depth_rects[0].x0, depth_rects[0].y1);
        draw_vector_line(depth_rects[1].x0, depth_rects[1].y0, depth_rects[1].x0, depth_rects[1].y1);
    }
    if (frustum.cells[0][2] == '#') {
        draw_braille_side_wall(depth_rects[0], depth_rects[1], 0, 1);
        draw_vector_line(depth_rects[0].x1, depth_rects[0].y0, depth_rects[0].x1, depth_rects[0].y1);
        draw_vector_line(depth_rects[1].x1, depth_rects[1].y0, depth_rects[1].x1, depth_rects[1].y1);
    }
    // Convert vector high-res map down to the target display matrix location panels
    flush_buffer_to_screen(right_panel_x, viewport_y);
    if (frustum.cells[1][1] == 'E' || frustum.cells[2][1] == 'E' || frustum.cells[3][1] == 'E') {
        mvprintw(viewport_y + (VIEW_H / 2), right_panel_x + (VIEW_W / 2) - 4, "[ ESCAPE ]");
    }
}

void render_game(Map * gm) {
    clear();

    int left_panel_x = 2;
    int left_panel_y = 2;

    int right_panel_x = (gm->cols * 2) + 6;
    int viewport_y = 2;

    // Panel A : 2D overview map
    // render_2D_map(left_panel_x);
    render_2D_map2(&game_map2, left_panel_x, left_panel_y);

    // divider between 2D and 3D views
    // for (int y = 0; y < gm->rows || y < VIEW_H + 4; y++) {
    //     mvaddch(y, right_panel_x - 3, '|');
    // }

    // PANEL B: Virtual Interface Framework
    // render_3D_view(right_panel_x, viewport_y );

    int footer_y = (gm->rows > VIEW_H + 4) ? gm->rows : VIEW_H + 4;
    mvprintw(footer_y + 1, 0, "Controls: UP/DOWN (Walk), LEFT/RIGHT (Turn). Press 'q' to quit.");
    refresh();
}

int main() {
    setlocale(LC_ALL, ""); // CRITICAL: Tell C runtime program system to unpack wide character sets
    if (!load_map("map.txt")) {
        printf("Error: Could not open map.txt.\n");
        return 1;
    }
    if (!load_map2("map2.txt")) {
        printf("Error: Could not open map2.txt.\n");
        return 1;
    }
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    // Inside main() after initscr():
    if (has_colors()) {
        start_color();
        // Pair 1: Text color doesn't matter much since we are filling the background
        init_pair(COLOR_WALL, COLOR_BLACK, COLOR_WHITE); // White walls
        init_pair(COLOR_TEXT, COLOR_WHITE, COLOR_BLACK); // Normal text
    }

    while (game_running) {
        render_game(&game_map);
        int ch = getch();
        switch (ch) {
            case KEY_UP: move_player(1);
                break;
            case KEY_DOWN: move_player(-1);
                break;
            case KEY_LEFT: turn_player(-1);
                break;
            case KEY_RIGHT: turn_player(1);
                break;
            case 'q': game_running = 0;
                break;
            default: break;
        }
    }
    endwin();
    if (player_won) printf("\n🎉 Victory! You smoothly outmaneuvered the vector maze wireframe layout!\n\n");
    return 0;
}
