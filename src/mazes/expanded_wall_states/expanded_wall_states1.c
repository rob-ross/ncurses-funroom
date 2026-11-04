//  expanded_wall_states1.c
//
//  Created by Rob Ross on 10/28/26.
//
//  Copyright (c) 2026.  All rights reserved.

// modified from frustum_utf8_01c to expand wall state to allow for visible and invisible doors.

//
// Apparently, using utf-8 or wide characters always requires ncursesw. However, both
// Apple and the official gnu implementation build the library as the wide ncursesw version.
// So even if you link against ncurses, you're really getting ncursesw. That's why utf-8 strings
// seemed to work in the non-wide version, but this was not a correct assumption.
// unicode characters REQUIRE:
// linking against ncursesw
// setting LOCALE to support utf-8.
// to use any wide-character APIs you must also #define _XOPEN_SOURCE_EXTENDED AND #include <ncursesw.h>


#define _XOPEN_SOURCE_EXTENDED 1 // Enforce POSIX and wide-character terminal extensions
#include <ncursesw/ncurses.h>

#include <assert.h>


#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#define MAX_ROWS 100
#define MAX_COLS 100

// NCURSES Character cell bounds
#define VIEW_W 40
#define VIEW_H 20

// High-Resolution Sub-Pixel Braille Canvas Map (2x4 pixels per text cell)
#define PIXEL_W (VIEW_W * 2) // 80 Pixels wide
#define PIXEL_H (VIEW_H * 4) // 80 Pixels high


// Define unique index handles for color configurations
#define COLOR_WALL 1
#define COLOR_TEXT 2

// in this order to make it easy to update position modulo 4
typedef enum { DIR_NORTH = 0, DIR_EAST = 1, DIR_SOUTH = 2, DIR_WEST = 3 } Direction;


/**
 * Bit flag masks to signify presence of a corresponding wall in a map cell.
 * Walls are encoded as W S E N
 * Bit:    7   6   |   5   4   |   3   2   |   1   0
 * Edge:  [ WEST ] |  [ SOUTH] |  [ EAST ] |  [ NORTH]
 */
typedef enum : unsigned char {
    EDGE_OPEN           = 0x0,
    EDGE_WALL           = 0x1,
    EDGE_DOOR           = 0x2,
    EDGE_INVISIBLE_DOOR = 0x3,
} EdgeType;

typedef unsigned char edge_flags;

#define EDGE_MASK 0x3

// Bit shifts for each cardinal direction (dir * 2)
#define SHIFT_NORTH 0
#define SHIFT_EAST  2
#define SHIFT_SOUTH 4
#define SHIFT_WEST  6

// typedef enum : unsigned char {
//     WM_NORTH = 1u << 0, // bit 0 (1)
//     WM_EAST  = 1u << 1, // bit 1 (2)
//     WM_SOUTH = 1u << 2, // bit 2 (4)
//     WM_WEST  = 1u << 3, // bit 3 (8)
// } WallFlags;

typedef enum : unsigned char {
    WM_NORTH = EDGE_MASK << 0,
    WM_EAST  = EDGE_MASK << 2,
    WM_SOUTH = EDGE_MASK << 4,
    WM_WEST  = EDGE_MASK << 6,
} WallMask;

typedef struct {
    int x;
    int y;
} Coord;

typedef struct {
    Coord pos;
    Direction dir;
} Player;


// What walls are visible from the current POV
typedef struct {
    edge_flags walls[4][3];
} ViewFrustum3;


// Models a map as a 2D array of gridsquares of equal size.
// Each gridsquare is a bit field containing the location of any of 4 walls surrounding the square
typedef struct {
    int rows;
    int cols;
    edge_flags walls[MAX_ROWS][MAX_COLS];
} MapData;


typedef struct {
    int x0, y0, x1, y1;
} WallRect;

typedef enum : int { FS_LEFT = 0, FS_CENTER = 1, FS_RIGHT = 2 } FrustumLane;


// temp globals for now, initialized by map loader
Coord start_location;
Coord end_location;


WallRect depth_rects[7] = {
    {0, 0, PIXEL_H - 1, PIXEL_W - 1 },    // Depth 0 (Near camera plane)
    {12, 12, 67, 67},  // Depth 1 (Boundary C0 / C1)
    {24, 24, 55, 55},  // Depth 2 (Boundary C1 / C2)
    {32, 32, 47, 47},  // Depth 3 (Boundary C2 / C3)
    {36, 36, 43, 43},  // Depth 4 (Boundary C3 / C4)
    {38, 38, 41, 41},  // Depth 5 (Boundary C4 / C5)
    {39, 39, 40, 40},  // Depth 6 (Boundary C5 / C4)

};


/*
 *  Initialized in init_joiner_map()
 *  The index for this lookup map is computed as follows:
 *  For a current cell C, the cell directly above A, and
 *  the cell directly to the left L, taking the flag bits (WM_NORTH, WM_EAST, WM_SOUTH, WM_WEST)
 *  for each that represent a wall location,
 *  index = C.WM_NORTH | C.WM_WEST | L.WM_NORTH | A.WM_WEST
 */
char * JOINERS_LOOKUP_MAP[16];
// = {
//     " ",
//     " ",
//     " ",
//     "┘",   // 2518
//     "│",   // 2502
//     "│",   // 2502
//     "┐",   // 2510
//     "┤",   // 2524
//     "─",   // 2500
//     "└",   // 2514
//     "─",   // 2500
//     "┴",   // 2534
//     "┌",   // 250C
//     "├",   // 251C
//     "┬",   // 252C
//     "┼",   // 253C
// };

constexpr char H_LINE[] = "\u2500"; //  "─"
constexpr char V_LINE[] = "\u2502"; //  "│"

// for horizontal door edges, ⊢   ⊣ are \u22A2 and \u22A3
// for vertical   door edges, ⊤   ⊥ are \u22A4 and \u22A5
// ⧧ 29E7
constexpr char H_DOOR_LEFT[]   = "\u22A3"; // ⊣
constexpr char H_DOOR_RIGHT[]  = "\u22A2"; // ⊢
constexpr char V_DOOR_TOP[]    = "\u22A5"; // ⊥
constexpr char V_DOOR_BOTTOM[] = "\u22A4"; // ⊤
constexpr char V_DOOR_SINGLE[] = "\U0001CC14"; // 𜰔  vertical capacitor
constexpr char HORIZONTAL_CAPACITOR[] = "\U0001CC13"; // 𜰓   horizontal capacitor
constexpr char VERTICAL_WHITE_RECTANGLE[] = "\u25AF"; // ▯
constexpr char HORIZONTAL_WHITE_RECTANGLE[] = "\u25AD"; // ▭



// Global Game State
MapData game_map;

Player player;
int game_running = 1;
int player_won = 0;

// The Virtual High-Resolution Graphics Byte-Buffer Screen
unsigned char pixel_buffer[PIXEL_H][PIXEL_W];


// -----------------------------------------------------------------
//      Forward References
// -----------------------------------------------------------------

ViewFrustum3 update_frustum3();



// make sure all flags are consistent with adjoining cells
bool validate_model(MapData * gm) {
    bool errs_found = false;
    for (int row = 0; row < gm->rows; ++row) {
        printf("Row %d: ", row);
        for (int col = 0; col < gm->cols; ++col) {
            edge_flags flags = gm->walls[row][col];
            //West/East check
            if ( col > 0 ) {
                edge_flags left_flags = gm->walls[row][col-1];
                if ( (bool)(flags & WM_WEST) != (bool)(left_flags & WM_EAST)) {
                    printf("col:%d: W/E mismatch, with col:%d, ", col, col -1 );
                    errs_found = true;
                }
            }

            //North/South check
            if (row > 0) {
                edge_flags above_flags = gm->walls[row-1][col];
                if ( (bool)(flags & WM_NORTH) != (bool)(above_flags & WM_SOUTH)) {
                    printf("col:%d: N/S mismatch with row:%d, col%d, ", col, row-1, col);
                    errs_found = true;
                }
            }

        }
        printf("\n");
    }
    fflush(stdout);
    return errs_found;
}

// Loads a text map file where each entry is  2-characters representing a hex byte from 00-FF
int load_map8t(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;
    game_map.rows = game_map.cols = 0;
    char line[MAX_COLS + 2];  // +2 to account for \n and \0 terminator

    while (fgets(line, sizeof(line), file) && game_map.rows < MAX_ROWS) {
        int c = 0;
        char *p = line;
        unsigned char cell;
        while (sscanf(p, "%2hhx", &cell) == 1 ) {
            game_map.walls[game_map.rows][c] = cell;
            c++;
            p+=2;
            if (*p == '\n' || *p == '\0') { break; }
            if (*p == ' ' ) { p++; } // eats the space
        }


        // while (line[c] != '\n' && line[c] != '\0' && c < MAX_COLS) {
        //     char grid_char =  line[c];
        //     // bad chars are interpreted as a solid wall square
        //     if (grid_char < '0' || grid_char > 'F') grid_char = 'F';
        //     unsigned char flags = strtol( (char[2]){ grid_char,'\0'}, nullptr, 16);
        //     game_map.walls[game_map.rows][c] = flags;
        //
        //     c++;
        // }
        if (c > game_map.cols) game_map.cols = c;
        game_map.rows++;
    }

    // useful utility to keep around
    // validate_model(&game_map);

    // todo we're currently hardcoding the entrance, exit for this map file
    start_location = (Coord){};

    player.pos = start_location;
    player.dir = DIR_NORTH;

    end_location.x = game_map.cols - 1;
    end_location.y = game_map.rows - 1;

    fclose(file);
    return 1;
}

// Loads a text map file where each character is a hex nibble 0-F.
// todo (rob) implement changes so we can load previous 4bit map files but use our 8bit map data structure
int load_map4t(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;
    game_map.rows = game_map.cols = 0;
    char line[MAX_COLS + 2];  // +2 to account for \n and \0 terminator
    while (fgets(line, sizeof(line), file) && game_map.rows < MAX_ROWS) {
        int c = 0;
        while (line[c] != '\n' && line[c] != '\0' && c < MAX_COLS) {
            char grid_char =  line[c];
            // bad chars are interpreted as a solid wall square
            if (grid_char < '0' || grid_char > 'F') grid_char = 'F';
            unsigned char flags = strtol( (char[2]){ grid_char,'\0'}, nullptr, 16);
            game_map.walls[game_map.rows][c] = flags;

            c++;
        }
        if (c > game_map.cols) game_map.cols = c;
        game_map.rows++;
    }

    // useful utility to keep around
    // validate_model(&game_map);

    // todo we're currently hardcoding the entrance, exit for this map file
    start_location = (Coord){};

    player.pos = start_location;
    player.dir = DIR_NORTH;

    end_location.x = game_map.cols - 1;
    end_location.y = game_map.rows - 1;

    fclose(file);
    return 1;

}

// unicode Box Drawing block, encoded as utf-8
void init_joiner_map() {
    JOINERS_LOOKUP_MAP[0]  = " ";
    JOINERS_LOOKUP_MAP[1]  = " ";
    JOINERS_LOOKUP_MAP[2]  = " ";
    JOINERS_LOOKUP_MAP[3]  = "┘";   // 2518
    JOINERS_LOOKUP_MAP[4]  = "│";   // 2502
    JOINERS_LOOKUP_MAP[5]  = "│";   // 2502
    JOINERS_LOOKUP_MAP[6]  = "┐";   // 2510
    JOINERS_LOOKUP_MAP[7]  = "┤";   // 2524
    JOINERS_LOOKUP_MAP[8]  = "─";   // 2500
    JOINERS_LOOKUP_MAP[9]  = "└";   // 2514
    JOINERS_LOOKUP_MAP[10] = "─";   // 2500
    JOINERS_LOOKUP_MAP[11] = "┴";   // 2534
    JOINERS_LOOKUP_MAP[12] = "┌";   // 250C
    JOINERS_LOOKUP_MAP[13] = "├";   // 251C
    JOINERS_LOOKUP_MAP[14] = "┬";   // 252C
    JOINERS_LOOKUP_MAP[15] = "┼";   // 253C
}

FILE *log_file = nullptr;

void init_log_file(const char * log_filename) {
    // 1. Open the file for writing ("w" overwrites, "a" appends)
    log_file = fopen(log_filename, "w");
}

const char * get_joiner_str(unsigned index) {
    assert( index <= 15 );
    return JOINERS_LOOKUP_MAP[index];
}

const char *  get_joiner_for_flags(bool north, bool west, bool left_north, bool above_west) {
    unsigned index = 0;
    index |= ( 1u * north << 3);
    index |= ( 1u * west  << 2);
    index |= ( 1u * left_north << 1);
    index |= ( 1u * above_west);
    return get_joiner_str(index);
}

// clear the edge_flags bit specified by `mask` for cell f[row][col]
void clear_wallflag(ViewFrustum3 *vf, int row, int col, WallMask mask) {
    assert(row >= 0 && row < MAX_ROWS && row < game_map.rows);
    assert(col >= 0 && col < MAX_COLS && col < game_map.cols);

    edge_flags *wf = &vf->walls[row][col];
    *wf = *wf & ~mask;
}

// Helper to extract the EdgeType for a given direction (relative or cardinal)
static inline EdgeType get_edge(edge_flags flags, Direction dir) {
    return (EdgeType)((flags >> (dir * 2)) & EDGE_MASK);
}

// Helpers for specific edge queries
static inline EdgeType get_north_edge(edge_flags flags) { return (EdgeType)(flags & 0x03); }
static inline EdgeType get_east_edge(edge_flags flags)  { return (EdgeType)((flags >> 2) & 0x03); }
static inline EdgeType get_south_edge(edge_flags flags) { return (EdgeType)((flags >> 4) & 0x03); }
static inline EdgeType get_west_edge(edge_flags flags)  { return (EdgeType)((flags >> 6) & 0x03); }

// Compresses 8-bit edge flags (2 bits/edge) down to 1 bit per present edge (non-zero)
static inline unsigned char get_presence_mask(edge_flags flags) {
    return (get_north_edge(flags) ? 1 : 0) |
          ((get_east_edge(flags)  ? 1 : 0) << 1) |
          ((get_south_edge(flags) ? 1 : 0) << 2) |
          ((get_west_edge(flags)  ? 1 : 0) << 3);
}

void write_wallflag_str(edge_flags wf, char * buffer) {
    const char n = wf & WM_NORTH ? 'N': ' ';
    const char e = wf & WM_EAST  ? 'E': ' ';
    const char s = wf & WM_SOUTH ? 'S': ' ';
    const char w = wf & WM_WEST  ? 'W': ' ';
    snprintf(buffer, 9, "%c %c %c %c", n, e, s, w);
}


void print_frustum_flags(ViewFrustum3 f) {
    // todo (rob) temp debug output
    // 1. Save tty modes and leave ncurses temporarily
    // def_prog_mode();
    // endwin();

    // 2. Now you are in regular mode: print freely to stdout
    fprintf(log_file, "\nPos: y:%d, x:%d\n", player.pos.y, player.pos.x);
    const char lane_prefix[] = {'L', 'C', 'R'};
    for (int depth = 3; depth >= 0 ; depth--) {
        fprintf(log_file, "D%d:  ", depth);
        for (int lane = 0; lane < 3; lane++) {
            char buffer[32] = {};
            write_wallflag_str(f.walls[depth][lane], buffer);
            fprintf(log_file, "[ %c%d: %s ]  ", lane_prefix[lane], depth, buffer);

        }
        fprintf(log_file,"\n");
    }
    fprintf(log_file, "\n");
    fflush(log_file);

    // 3. Return to ncurses mode
    // reset_prog_mode();
    // refresh();            // Restore the ncurses screen contents
}


void printf_view_frustum() {
    print_frustum_flags(update_frustum3());
}

//// ------------------------------------------------------------
////
////    3D Rendering
////
//// ------------------------------------------------------------


// Clear the vector canvas before each rendering update pass
void clear_pixel_buffer() {
    // todo (rob) use memset
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

void draw_lateral_facing_wall(WallRect r, bool is_left) {
    int cell_w = r.x1 - r.x0;
    if (is_left) {
        int lx0 = (r.x0 - cell_w < 0) ? 0 : r.x0 - cell_w;
        int lx1 = r.x0;
        draw_vector_line(lx0, r.y0, lx1, r.y0); // Top
        draw_vector_line(lx0, r.y1, lx1, r.y1); // Bottom
        draw_vector_line(lx1, r.y0, lx1, r.y1); // Inner vertical post at corridor boundary
        if (lx0 > 0) draw_vector_line(lx0, r.y0, lx0, r.y1); // Outer vertical boundary
    } else {
        int rx0 = r.x1;
        int rx1 = (r.x1 + cell_w >= PIXEL_W) ? PIXEL_W - 1 : r.x1 + cell_w;
        draw_vector_line(rx0, r.y0, rx1, r.y0); // Top
        draw_vector_line(rx0, r.y1, rx1, r.y1); // Bottom
        draw_vector_line(rx0, r.y0, rx0, r.y1); // Inner vertical post at corridor boundary
        if (rx1 < PIXEL_W - 1) draw_vector_line(rx1, r.y0, rx1, r.y1); // Outer vertical boundary
    }
}

// Computes the WallRect for a door centered within a front wall
WallRect get_door_rect(WallRect wall) {
    int wall_w = wall.x1 - wall.x0;
    int wall_h = wall.y1 - wall.y0;

    int door_w = (wall_w * 1) / 2;
    int door_h = (wall_h * 2) / 3;

    int offset_x = (wall_w - door_w) / 2;

    WallRect door;
    door.x0 = wall.x0 + offset_x;
    door.x1 = door.x0 + door_w;
    door.y1 = wall.y1;
    door.y0 = wall.y1 - door_h;

    return door;
}

// Renders a door frame scaled to the appropriate wall depth
void draw_braille_door(WallRect wall) {
    WallRect door = get_door_rect(wall);

    // Top lintel
    draw_vector_line(door.x0, door.y0, door.x1, door.y0);
    // Left post
    draw_vector_line(door.x0, door.y0, door.x0, door.y1);
    // Right post
    draw_vector_line(door.x1, door.y0, door.x1, door.y1);
    // Bottom threshold (optional if floor line is already drawn by the parent wall)
    draw_vector_line(door.x0, door.y1, door.x1, door.y1);
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
            // we could put all possible 256 braille values in an array for a lookup table, but that requires
            // at least one pointer dereference, so bit shifting here may be faster.
            char utf8_str[4];
            utf8_str[0] = (char)(0xE0 | ((braille_char_offset >> 12) & 0x0F)); // 0xE2 for Braille
            utf8_str[1] = (char)(0x80 | ((braille_char_offset >> 6)  & 0x3F)); // 0xA0..0xA3
            utf8_str[2] = (char)(0x80 | (braille_char_offset         & 0x3F)); // 0x80..0xBF
            utf8_str[3] = '\0';

            move(start_y + ch_y, start_x + ch_x);
            addstr(utf8_str);
        }
    }
}


// translate a WallFlags bitfield based on the argument `dir` orientation
// New 8-bit (2 bits per edge):
edge_flags rotate_walls(edge_flags flags, Direction dir) {
    unsigned int duplicated = (unsigned int)flags | ((unsigned int)flags << 8);
    return (edge_flags)((duplicated >> (dir * 2)) & 0xFF);
}

edge_flags get_walls_at3(Coord target, Direction dir) {
    if (target.x < 0 || target.x >= game_map.cols || target.y < 0 || target.y >= game_map.rows) {
        return 0;
    }
    edge_flags target_flags = game_map.walls[target.y][target.x];
    edge_flags f = rotate_walls(target_flags, dir); // these are *all* the walls that exist in the frustum.
    return f;
}



ViewFrustum3 update_frustum3() {
    // todo (rob) make size of frustum dynamic so we can see more or less depending on other conditions.
    ViewFrustum3 f;
    int frustum_depth = 4; // this is currently hardcoded in ViewFrustum3 as 4.

    // we look in the current cell for existing/visible walls
    // When we face North, we're looking at the North wall
    //
    // first we capture all the walls that exist in our field of view
    int fx = 0, fy = 0, rx = 0, ry = 0;
    switch (player.dir) {
        case DIR_NORTH:
            fx = 0;
            fy = -1;
            rx = 1;
            ry = 0;
            break;
        case DIR_EAST:
            fx = 1;
            fy = 0;
            rx = 0;
            ry = 1;
            break;
        case DIR_SOUTH:
            fx = 0;
            fy = 1;
            rx = -1;
            ry = 0;
            break;
        case DIR_WEST:
            fx = -1;
            fy = 0;
            rx = 0;
            ry = -1;
            break;
    }

    for (int depth = 0; depth < frustum_depth; depth++) {
        for (int lane = -1; lane <= 1; lane++) {
            // when lane == -1, we're looking Left
            // when lane ==  0, we're looking center
            // when lane ==  1, we're looking right
            Coord target = {
                player.pos.x + (fx * depth) + (rx * lane),
                player.pos.y + (fy * depth) + (ry * lane)
            };
            f.walls[depth][lane + 1] = get_walls_at3(target, player.dir);
        }
    }



    // todo (rob) we need to eliminate walls that are not visible.
    // we can't see the outermost walls of L0 nor R0
    clear_wallflag(&f, 0, FS_LEFT,  WM_WEST);
    clear_wallflag(&f, 0, FS_RIGHT,  WM_EAST);

    // if there are walls to our immediate left or right, we can't see the corresponding north wall of that cell
    for (int depth = 0; depth < frustum_depth; depth++) {
        if (f.walls[depth][FS_CENTER] & WM_WEST) {
            clear_wallflag(&f, depth, FS_LEFT, WM_NORTH);
        }
        if (f.walls[depth][FS_CENTER] & WM_EAST) {
            clear_wallflag(&f, depth, FS_RIGHT, WM_NORTH);
        }
    }


    // -----------------------------------------------------------------
    // Depth 0
    // -----------------------------------------------------------------

    //  South walls are behind player and not visible.
    clear_wallflag(&f, 0, FS_LEFT,  WM_SOUTH);
    clear_wallflag(&f, 0, FS_CENTER, WM_SOUTH);
    clear_wallflag(&f, 0, FS_RIGHT, WM_SOUTH);

    if (f.walls[0][1] & WM_NORTH) {
        // all walls in FS_CENTER past this cell are obscured.
        for (int depth = 1; depth < frustum_depth; depth++) {
            for (int lane = 0; lane < 3; lane++) {
                f.walls[depth][lane ] = 0;
            }
        }
    }

    // -----------------------------------------------------------------
    // Depth 1
    // -----------------------------------------------------------------

    if (f.walls[1][FS_CENTER] & WM_NORTH) {
        // if C1.West or L1.North, prune the entire left lane
        if ( (f.walls[1][FS_CENTER] & WM_WEST) || (f.walls[1][FS_LEFT] & WM_NORTH) ) {
            for (int depth = 2; depth < frustum_depth; depth++) {
                f.walls[depth][FS_LEFT ] = 0;
            }
        }
        // similarly for the right lane
        if ( (f.walls[1][FS_CENTER] & WM_EAST) || (f.walls[1][FS_RIGHT] & WM_NORTH) ) {
            for (int depth = 2; depth < frustum_depth; depth++) {
                f.walls[depth][FS_RIGHT ] = 0;
            }
        }
    }

    // -----------------------------------------------------------------
    // Depth 2
    // -----------------------------------------------------------------
    if (f.walls[2][FS_CENTER] & WM_NORTH)
        // if C1.West or L1.North, prune the entire left lane
            if ( (f.walls[2][FS_CENTER] & WM_WEST) || (f.walls[2][FS_LEFT] & WM_NORTH) ) {
                for (int depth = 3; depth < frustum_depth; depth++) {
                    f.walls[depth][FS_LEFT ] = 0;
                }
            }
    // similarly for the right lane
    if ( (f.walls[2][FS_CENTER] & WM_EAST) || (f.walls[2][FS_RIGHT] & WM_NORTH) ) {
        for (int depth = 3; depth < frustum_depth; depth++) {
            f.walls[depth][FS_RIGHT ] = 0;
        }
    }


    return f;
}

void render_3D_view2(int right_panel_x, int viewport_y) {
        // PANEL B: Virtual Interface Framework

    ViewFrustum3 frustum = update_frustum3();
    clear_pixel_buffer(); // Fresh slate step for vector processing buffer canvas


    mvprintw(0, right_panel_x, "--- 3D BRAILLE VECTOR ENGINE ---");

    // try drawing a tight border around the 3D view canvas
    // ----------------------------------------------------
    int result = {};
    // move(viewport_y-1, right_panel_x-1);
    // result = hline('-', VIEW_W + 2);   // top line
    // result = vline('|', VIEW_H + 2);   // left line
    // move(viewport_y  + VIEW_H + 1, right_panel_x -1 );
    // result = hline('-', VIEW_W + 2);   // bottom line
    // move(viewport_y-1, right_panel_x + 1 + VIEW_W);
    // result = vline('|', VIEW_H + 2);   // right line
    // todo (rob) temp debug draw outline around the inner edges of the viewport
    move(viewport_y, right_panel_x);
    result = hline('-', VIEW_W);   // top line
    result = vline('|', VIEW_H);   // left line
    move(viewport_y  + VIEW_H, right_panel_x );
    result = hline('-', VIEW_W - 1);   // bottom line
    move(viewport_y, right_panel_x + VIEW_W - 1);
    result = vline('|', VIEW_H );   // right line


    // --- HIGH-RESOLUTION RASTERIZATION DRAW PIPELINE ---


    // -----------------------------------------------------------------
    //      LAYER 3: Far Distance (Depth 3)
    // -----------------------------------------------------------------

    if (frustum.walls[3][FS_CENTER] & WM_WEST) {
        draw_braille_side_wall(depth_rects[3], depth_rects[4], 1, 0);
        draw_vector_line(depth_rects[3].x0, depth_rects[3].y0, depth_rects[3].x0, depth_rects[3].y1); // Near pillar (z=3)
        draw_vector_line(depth_rects[4].x0, depth_rects[4].y0, depth_rects[4].x0, depth_rects[4].y1); // Far pillar  (z=4)
    }
    if (frustum.walls[3][FS_CENTER] & WM_EAST) {
        draw_braille_side_wall(depth_rects[3], depth_rects[4], 0, 1);
        draw_vector_line(depth_rects[3].x1, depth_rects[3].y0, depth_rects[3].x1, depth_rects[3].y1); // Near pillar (z=3)
        draw_vector_line(depth_rects[4].x1, depth_rects[4].y0, depth_rects[4].x1, depth_rects[4].y1); // Far pillar  (z=4)
    }
    if (frustum.walls[3][FS_CENTER] & WM_NORTH) {
        clear_braille_sub_rect(depth_rects[4]);
        draw_braille_front_wall(depth_rects[4]);
        EdgeType front_edge = get_north_edge(frustum.walls[3][FS_CENTER]);
        if (front_edge == EDGE_DOOR) {
            // Draw the inset door
            draw_braille_door(depth_rects[4]);
        }
    }
    if (frustum.walls[3][FS_LEFT] & WM_NORTH ) {
        draw_lateral_facing_wall(depth_rects[4], true);
    }
    if (frustum.walls[3][FS_RIGHT] & WM_NORTH ) {
        draw_lateral_facing_wall(depth_rects[4], false);
    }

    // -----------------------------------------------------------------
    //      LAYER 2: Mid Distance (Depth 2)
    // -----------------------------------------------------------------

    if (frustum.walls[2][FS_CENTER] & WM_WEST) {
        draw_braille_side_wall(depth_rects[2], depth_rects[3], 1, 0);
        draw_vector_line(depth_rects[2].x0, depth_rects[2].y0, depth_rects[2].x0, depth_rects[2].y1);
        draw_vector_line(depth_rects[3].x0, depth_rects[3].y0, depth_rects[3].x0, depth_rects[3].y1);
    }
    if (frustum.walls[2][FS_CENTER] & WM_EAST ) {
        draw_braille_side_wall(depth_rects[2], depth_rects[3], 0, 1);
        draw_vector_line(depth_rects[2].x1, depth_rects[2].y0, depth_rects[2].x1, depth_rects[2].y1);
        draw_vector_line(depth_rects[3].x1, depth_rects[3].y0, depth_rects[3].x1, depth_rects[3].y1);
    }
    if (frustum.walls[2][FS_CENTER] & WM_NORTH ) {
        clear_braille_sub_rect(depth_rects[3]);
        draw_braille_front_wall(depth_rects[3]);
        EdgeType front_edge = get_north_edge(frustum.walls[2][FS_CENTER]);
        if (front_edge == EDGE_DOOR) {
            // Draw the inset door
            draw_braille_door(depth_rects[3]);
        }
    }
    if (frustum.walls[2][FS_LEFT] & WM_NORTH ) {
        draw_lateral_facing_wall(depth_rects[3], true);
    }
    if (frustum.walls[2][FS_RIGHT] & WM_NORTH ) {
        draw_lateral_facing_wall(depth_rects[3], false);
    }


    // -----------------------------------------------------------------
    //      LAYER 1: Close Distance (Depth 1)
    // -----------------------------------------------------------------

    if (frustum.walls[1][FS_CENTER] & WM_WEST ) {
        draw_braille_side_wall(depth_rects[1], depth_rects[2], 1, 0);
        draw_vector_line(depth_rects[1].x0, depth_rects[1].y0, depth_rects[1].x0, depth_rects[1].y1);
        draw_vector_line(depth_rects[2].x0, depth_rects[2].y0, depth_rects[2].x0, depth_rects[2].y1);
    }
    if (frustum.walls[1][FS_CENTER] & WM_EAST ) {
        draw_braille_side_wall(depth_rects[1], depth_rects[2], 0, 1);
        draw_vector_line(depth_rects[1].x1, depth_rects[1].y0, depth_rects[1].x1, depth_rects[1].y1);
        draw_vector_line(depth_rects[2].x1, depth_rects[2].y0, depth_rects[2].x1, depth_rects[2].y1);
    }
    if (frustum.walls[1][FS_CENTER] & WM_NORTH ) {
        clear_braille_sub_rect(depth_rects[2]);
        draw_braille_front_wall(depth_rects[2]);
        EdgeType front_edge = get_north_edge(frustum.walls[1][FS_CENTER]);
        if (front_edge == EDGE_DOOR) {
            // Draw the inset door
            draw_braille_door(depth_rects[2]);
        }
    }
    if (frustum.walls[1][FS_LEFT] & WM_NORTH ) {
        draw_lateral_facing_wall(depth_rects[2], true);
    }
    if (frustum.walls[1][FS_RIGHT] & WM_NORTH ) {
        draw_lateral_facing_wall(depth_rects[2], false);
    }


    // -----------------------------------------------------------------
    //  LAYER 0: Immediate walls surrounding player coordinates
    // -----------------------------------------------------------------

    if (frustum.walls[0][FS_CENTER] & WM_WEST ) {
        draw_braille_side_wall(depth_rects[0], depth_rects[1], 1, 0);
        draw_vector_line(depth_rects[0].x0, depth_rects[0].y0, depth_rects[0].x0, depth_rects[0].y1);
        draw_vector_line(depth_rects[1].x0, depth_rects[1].y0, depth_rects[1].x0, depth_rects[1].y1);
    }
    if (frustum.walls[0][FS_CENTER] & WM_EAST ) {
        draw_braille_side_wall(depth_rects[0], depth_rects[1], 0, 1);
        draw_vector_line(depth_rects[0].x1, depth_rects[0].y0, depth_rects[0].x1, depth_rects[0].y1);
        draw_vector_line(depth_rects[1].x1, depth_rects[1].y0, depth_rects[1].x1, depth_rects[1].y1);
    }
    if (frustum.walls[0][FS_CENTER] & WM_NORTH) {
        clear_braille_sub_rect(depth_rects[1]);
        draw_braille_front_wall(depth_rects[1]);
        EdgeType front_edge = get_north_edge(frustum.walls[0][FS_CENTER]);
        if (front_edge == EDGE_DOOR) {
            // Draw the inset door
            draw_braille_door(depth_rects[1]);
        }
    }
    if (frustum.walls[0][FS_LEFT] & WM_NORTH ) {
        draw_lateral_facing_wall(depth_rects[1], true);
    }
    if (frustum.walls[0][FS_RIGHT] & WM_NORTH ) {
        draw_lateral_facing_wall(depth_rects[1], false);
    }

    // Convert vector high-res map down to the target display matrix location panels
    flush_buffer_to_screen(right_panel_x, viewport_y);


}



//// ------------------------------------------------------------
////
////    2D Drawing
////
//// ------------------------------------------------------------

void render_2D_map2(MapData * gm, int left_panel_x, int left_panel_y) {
    // in the Map model, adjacent cells share a common wall. I.e, the East wall of a cell will have the
    // same value as the West wall of the cell to the right. We use this to optimize drawing by only
    // processing the North and West walls and the North West joiners in the main rendering loop,
    // and dealing with the last row and last columns separately to draw
    // the rightmost and bottom most walls of the map.
    //
    // PANEL A: 2D Minimap Overhead
    // each view "square" is 5 terminal cells wide and 3 cells tall. The top, bottom, left, and right cells
    // are for wall elements. Each view square has 3 terminal cells for displaying markers such as player position
    // start, end points, etc.
    int col_scale = 5; // we scale the grid square up by this amount for cols
    int row_scale = 3;




    // This first loop draws all North and West walls and joiners for the North-West corner of each cell.
    // The right and bottom edges of the map require special attention since there are no map
    // cells to the right or bottom of these cells.

    for (int y = 0; y < gm->rows; y++) {
        for (int x = 0; x < gm->cols; x++) {
            // draw gridsquare borders
            edge_flags flags = gm->walls[y][x];
            // screen_x and _screen_y should be the scaled values of the top-left
            // coordinate of each terminal square

            const int screen_x = left_panel_x + (x * (col_scale - 1))  ;
            const int screen_y = left_panel_y + (y * (row_scale - 1))  ;

            // Top edges
            EdgeType et;
            if ( (et = get_north_edge(flags))) {
                for (int i = 1; i < col_scale - 1  ; ++i) {
                    mvaddstr(screen_y, screen_x+i, H_LINE);
                }
                if (et == EDGE_DOOR) {
                    mvaddstr(screen_y, screen_x + 2, " ");

                    // mvaddstr(screen_y, screen_x + 1, H_DOOR_LEFT);
                    // mvaddstr(screen_y, screen_x + col_scale - 2, H_DOOR_RIGHT);
                    // mvaddstr(screen_y, screen_x + 1, "⊨");
                    // mvaddstr(screen_y, screen_x + col_scale - 2, "⊧");
                }
            }

            // Left edges
            if ( (et = get_west_edge(flags) )) {
                if (et == EDGE_DOOR) {
                    // mvaddstr(screen_y + 1 , screen_x, "╬");
                    mvaddstr(screen_y + 1 , screen_x, " ");

                } else {
                    for (int i = 1; i < row_scale - 1 ; ++i) {
                        mvaddstr(screen_y + i, screen_x , V_LINE);
                    }
                }
            }

            edge_flags above_flags = (y <= 0 ) ? 0 : gm->walls[y-1][x];
            edge_flags left_flags  = (x <= 0 ) ? 0 : gm->walls[y][x-1];
            const char * north_west_joiner_str = get_joiner_for_flags(
                flags & WM_NORTH, flags & WM_WEST, left_flags & WM_NORTH, above_flags & WM_WEST  );

            mvaddstr(screen_y, screen_x, north_west_joiner_str);
        }
    } // main iteration



    // last row
    // Draws the South walls and South West joiner of each cell
    for (int x = 0; x < gm->cols; x++) {
        const int y = gm->rows-1;
        const int screen_x = left_panel_x + (x * (col_scale - 1)) ;
        const int screen_y = left_panel_y + (y * (row_scale - 1)) ;

        edge_flags flags = gm->walls[y][x];
        edge_flags above_flags = (y <= 0 ) ? 0 : gm->walls[y-1][x];
        edge_flags left_flags  = (x <= 0 ) ? 0 : gm->walls[y][x-1];
            // we have to render the south edges
            if ( get_south_edge(flags)) {
                for (int i = 0; i < col_scale - 1; ++i) {
                    mvaddstr(screen_y + row_scale - 1, screen_x+i, H_LINE);
                }
            }
            const char * south_west_joiner_str = nullptr;
            south_west_joiner_str = get_joiner_for_flags(
                        flags & WM_SOUTH, 0, left_flags & WM_SOUTH, flags & WM_WEST  );
            mvaddstr(screen_y + row_scale - 1, screen_x , south_west_joiner_str);
    }

    // last column of each row
    // Draws the East walls and the North East joiners of each cell
    for (int y = 0; y < gm->rows; y++) {
        const int x = gm->cols-1;
        const int screen_x = left_panel_x + (x * (col_scale - 1)) ;
        const int screen_y = left_panel_y + (y * (row_scale - 1)) ;
        edge_flags flags = gm->walls[y][x];
        edge_flags above_flags = (y <= 0 ) ? 0 : gm->walls[y-1][x];
        edge_flags left_flags  = (x <= 0 ) ? 0 : gm->walls[y][x-1];
        edge_flags below_flags  = (y <= 0 ) ? 0 : gm->walls[y+1][x];

        if ( get_east_edge(flags) ) {
            for (int i = 1; i < row_scale - 1; ++i) {
                mvaddstr(screen_y + i, screen_x + col_scale - 1 , V_LINE);
            }
        }
        // getch();
        //joiners
        // top joiner
        const char * north_east_joiner_str = get_joiner_for_flags(
        0, flags & WM_EAST, flags & WM_NORTH, above_flags & WM_EAST  );
        mvaddstr(screen_y, screen_x + col_scale - 1, north_east_joiner_str);


        // last column of last row
        // Render joiner for bottom right corner of bottom-right map cell
        if ( y == gm->rows - 1 ) {
            const char * south_east_joiner_str = get_joiner_for_flags(
            0, 0, flags & WM_SOUTH, flags & WM_EAST  );
            mvaddstr(screen_y + row_scale - 1, screen_x + col_scale - 1, south_east_joiner_str);
        }
    }


    // Draw Player Avatar
    // utf-8 implementation
    char *utf8_avatars[4] = {"▲","▶","▼","◀"};
    mvaddstr(
        left_panel_y + player.pos.y * ( row_scale -1 ) + 1,
        left_panel_x + (player.pos.x * ( col_scale - 1 )) + 2,
        utf8_avatars[player.dir]
    );

    // Draw Player Avatar
    // Wide-Char implementation

    /*// const uint32_t avatar_unicode[] = { L'▲', L'▶', L'▼', L'◀'};

    const cchar_t new_avatar_chars[] = {
        { .chars = L'▲'},
         { .chars = L'▶'},
        { .chars = L'▼'},
        { .chars = L'◀'}
    };
    mvadd_wch(
        left_panel_y + player.pos.y * ( row_scale -1 ) + 1,
        left_panel_x + (player.pos.x * ( col_scale - 1 )) + 2,
        &new_avatar_chars[player.dir]
    );*/

    // Draw Player Avatar
    // ASCII Char Implementation
    /*
    char avatars[] = {'^', '>', 'v', '<' };
    mvaddch(
        left_panel_y + player.pos.y * ( row_scale -1 ) + 1,
        left_panel_x + (player.pos.x * ( col_scale - 1 )) + 2,
        avatars[player.dir]
    );
    */


}


// -----------------------------------------------------------------
//      Movement
// -----------------------------------------------------------------

bool player_can_move(Player p, MapData * gm, Coord target, Direction dir) {
    if (target.x < 0 || target.x >= gm->cols || target.y < 0 || target.y >= gm->rows) return false;
    edge_flags flags = gm->walls[p.pos.y][p.pos.x];
    EdgeType edge = get_edge(flags, dir);
    if (edge == EDGE_OPEN || edge == EDGE_DOOR || edge == EDGE_INVISIBLE_DOOR) {
        return true;
    }
    // switch (dir) {
    //     case WM_SOUTH: return !(flags & WM_SOUTH);
    //         break;
    //     case WM_NORTH: return !(flags & WM_NORTH);
    //         break;
    //     case WM_EAST:  return !(flags & WM_EAST);
    //         break;
    //     case WM_WEST:  return !(flags & WM_WEST);
    //         break;
    // }

    return false;
}

void turn_player(int rotation) { player.dir = (player.dir + rotation + 4) % 4; }

void move_player(int step) {
    int dx = 0, dy = 0;
    WallMask check_wall = {};

    switch (player.dir) {
        case DIR_NORTH:
            dy = -step;
            check_wall = dy < 0 ? WM_NORTH : WM_SOUTH;
            break;
        case DIR_SOUTH:
            dy = step;
            check_wall = dy < 0 ? WM_NORTH : WM_SOUTH;
            break;
        case DIR_EAST:
            dx = step;
            check_wall = dx < 0 ? WM_WEST : WM_EAST;
            break;
        case DIR_WEST:
            dx = -step;
            check_wall = dx < 0 ? WM_WEST : WM_EAST;
            break;
    }
    Coord target = {player.pos.x + dx, player.pos.y + dy};
    if (player_can_move(player, &game_map, target, player.dir)) {
        player.pos = target;
        if (player.pos.x == end_location.x &&
            player.pos.y == end_location.y )
        {
            // do end code
            // player_won = 1;
            // game_running = 0;
        }
    }
}

void render_game(MapData * gm) {
    // clear();
    erase();

    int left_panel_x = 2;
    int left_panel_y = 2;

    int right_panel_x = (gm->cols * 5 - 1) + 6;
    int viewport_y = 2;

    // Panel A : 2D overview map
    // render_2D_map(left_panel_x);
    render_2D_map2(&game_map, left_panel_x, left_panel_y);

    // divider between 2D and 3D views
    // for (int y = 0; y < gm->rows || y < VIEW_H + 4; y++) {
    //     mvaddch(y, right_panel_x - 3, '|');
    // }

    // PANEL B: Virtual Interface Framework
    render_3D_view2(right_panel_x, viewport_y );

    int win_h, win_w;
    getmaxyx(stdscr, win_h, win_w);
    // int footer_y = (gm->rows > win_h + 4) ? gm->rows : win_w + 4;
    mvprintw(win_h - 1, 0, "Controls: UP/DOWN (Walk), LEFT/RIGHT (Turn). Press 'q' to quit.");
    refresh();
}


int main(int argc, char *argv[]) {
    int return_code = 0;
    // CRITICAL: Tell C runtime program system to unpack wide character sets
    // 1. Try to inherit the user's environment
    if (!setlocale(LC_ALL, "")) {
        // 2. Fallback to standard C.UTF-8 or UTF-8 if the environment is misconfigured
        if (!setlocale(LC_ALL, "C.UTF-8")) {
            setlocale(LC_ALL, "en_US.UTF-8");
        }
    }

    init_log_file("flog.txt");

    const char * default_filename = "./maps/fmap_14x14_empty.map8t";
    const char * filename = default_filename;
    if (argc > 1) {
        // first argument is the name of the map file to use
        filename = argv[1];
    }

    if (!load_map8t(filename)) {
        printf("Error: Could not open %s.\n", filename);
        return_code =  1;
        goto end;
    }

    init_joiner_map();

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
            case 'p' : printf_view_frustum();
                break;
            case 'q': game_running = 0;
                break;
            default: break;
        }
    }
    endwin();

    if (player_won) printf("\n🎉 Victory! You smoothly outmaneuvered the vector maze wireframe layout!\n\n");

end:
    if (log_file) {
        fclose(log_file);
        log_file = nullptr;
    }


    return return_code;
}
