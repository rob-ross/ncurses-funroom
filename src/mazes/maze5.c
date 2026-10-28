//  maze5.c
//
//  Created by Rob Ross on 10/28/26.
//
//  Copyright (c) 2026.  All rights reserved.

// we adjust the horizontal scale so each map square occupies 2 horizontal cells


#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ROWS 100
#define MAX_COLS 100

// Viewport Dimensions on Right Panel
#define VIEW_W 40
#define VIEW_H 20



// Define unique index handles for color configurations
#define COLOR_WALL 1
#define COLOR_TEXT 2

typedef enum {
    DIR_NORTH = 0,
    DIR_EAST  = 1,
    DIR_SOUTH = 2,
    DIR_WEST  = 3
} Direction;

typedef struct { int x; int y; } Coord;
typedef struct { Coord pos; Direction dir; } Player;
typedef struct { char grid[MAX_ROWS][MAX_COLS]; int rows; int cols; } Map;
typedef struct { char cells[4][3]; } ViewFrustum;

// Wireframe Layer Geometry Config
typedef struct { int x0, y0, x1, y1; } WallRect;

// Precalculated box footprints for Depths 1, 2, and 3
WallRect depth_rects[4] = {
    { 0,  0,  39, 19 }, // Depth 0 (Screen edge bounds)
    { 6,  3,  33, 16 }, // Depth 1 (Close)
    { 12, 6,  27, 13 }, // Depth 2 (Mid)
    { 16, 8,  23, 11 }  // Depth 3 (Far)
};
// Scaled 2:1 Aspect Ratio Depth Rectangles
// WallRect depth_rects[] = {
//     { 0,  0,  79, 19 }, // Depth 0 (Viewport container bounds)
//     { 12, 3,  67, 16 }, // Depth 1 (Close wall layer)
//     { 24, 6,  55, 13 }, // Depth 2 (Mid-distance layer)
//     { 32, 8,  47, 11 }  // Depth 3 (Far-distance layer)
// };
// Corrected 2:1 Aspect Ratio Depth Rectangles
// WallRect depth_rects[] = {
//     { 0,  0,  79, 19 }, // Depth 0 (Viewport container bounds)
//     { 20, 3,  59, 16 }, // Depth 1 (Close Wall: Height=13, Width=39 -> Perfect 2:1 Square)
//     { 30, 6,  49, 13 }, // Depth 2 (Mid Wall:   Height=7,  Width=19 -> Perfect 2:1 Square)
//     { 34, 8,  45, 11 }  // Depth 3 (Far Wall:   Height=3,  Width=11 -> Perfect 2:1 Square)
// };



Map game_map;
Player player;
int game_running = 1;
int player_won = 0;

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
                player.pos.x = c; player.pos.y = game_map.rows;
                player.dir = DIR_NORTH; game_map.grid[game_map.rows][c] = ' ';
            }
            c++;
        }
        if (c > game_map.cols) game_map.cols = c;
        game_map.rows++;
    }
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
        case DIR_NORTH: fx =  0; fy = -1; rx =  1; ry =  0; break;
        case DIR_EAST:  fx =  1; fy =  0; rx =  0; ry =  1; break;
        case DIR_SOUTH: fx =  0; fy =  1; rx = -1; ry =  0; break;
        case DIR_WEST:  fx = -1; fy =  0; rx =  0; ry = -1; break;
    }
    for (int depth = 0; depth <= 3; depth++) {
        for (int offset = -1; offset <= 1; offset++) {
            Coord target = { player.pos.x + (fx * depth) + (rx * offset), player.pos.y + (fy * depth) + (ry * offset) };
            f.cells[depth][offset + 1] = get_tile_at(target);
        }
    }
    return f;
}

void turn_player(int rotation) { player.dir = (player.dir + rotation + 4) % 4; }
void move_player(int step) {
    int dx = 0, dy = 0;
    switch (player.dir) {
        case DIR_NORTH: dy = -step; break; case DIR_SOUTH: dy =  step; break;
        case DIR_EAST:  dx =  step; break; case DIR_WEST:  dx = -step; break;
    }
    Coord target = { player.pos.x + dx, player.pos.y + dy };
    char tile = get_tile_at(target);
    if (tile == ' ') { player.pos = target; }
    else if (tile == 'E') { player.pos = target; player_won = 1; game_running = 0; }
}

// Draw a flat facing front wall rectangle
void draw_front_wall(int start_x, int start_y, WallRect r) {
    for (int x = r.x0; x <= r.x1; x++) {
        mvaddch(start_y + r.y0, start_x + x, ACS_HLINE);
        mvaddch(start_y + r.y1, start_x + x, ACS_HLINE);
    }
    for (int y = r.y0; y <= r.y1; y++) {
        mvaddch(start_y + y, start_x + r.x0, ACS_VLINE);
        mvaddch(start_y + y, start_x + r.x1, ACS_VLINE);
    }
    mvaddch(start_y + r.y0, start_x + r.x0, ACS_ULCORNER);
    mvaddch(start_y + r.y0, start_x + r.x1, ACS_URCORNER);
    mvaddch(start_y + r.y1, start_x + r.x0, ACS_LLCORNER);
    mvaddch(start_y + r.y1, start_x + r.x1, ACS_LRCORNER);
}

// Fill perspective side extensions between wall depth transitions
void draw_side_wall_lines(int start_x, int start_y, WallRect near_r, WallRect far_r, int left_side, int right_side) {
    if (left_side) {
        // Ceiling angle line
        for (int x = near_r.x0; x <= far_r.x0; x++) {
            int y = near_r.y0 + (x - near_r.x0) * (far_r.y0 - near_r.y0) / (far_r.x0 - near_r.x0);
            mvaddch(start_y + y, start_x + x, '\\');
        }
        // Floor angle line
        for (int x = near_r.x0; x <= far_r.x0; x++) {
            int y = near_r.y1 - (x - near_r.x0) * (near_r.y1 - far_r.y1) / (far_r.x0 - near_r.x0);
            mvaddch(start_y + y, start_x + x, '/');
        }
    }
    if (right_side) {
        // Ceiling angle line
        for (int x = far_r.x1; x <= near_r.x1; x++) {
            int y = far_r.y0 + (x - far_r.x1) * (near_r.y0 - far_r.y0) / (near_r.x1 - far_r.x1);
            mvaddch(start_y + y, start_x + x, '/');
        }
        // Floor angle line
        for (int x = far_r.x1; x <= near_r.x1; x++) {
            int y = far_r.y1 - (x - far_r.x1) * (far_r.y1 - near_r.y1) / (near_r.x1 - far_r.x1);
            mvaddch(start_y + y, start_x + x, '\\');
        }
    }
}


// Fill a corridor frame space with a blank void texture
void clear_sub_rect(int start_x, int start_y, WallRect r) {
    for (int y = r.y0 + 1; y < r.y1; y++) {
        for (int x = r.x0 + 1; x < r.x1; x++) {
            mvaddch(start_y + y, start_x + x, ' ');
        }
    }
}

void render_game() {
    clear();
    ViewFrustum frustum = update_frustum();

    int left_panel_x = 0;
    int right_panel_x = game_map.cols*2 + 6;
    int viewport_y = 2;

    // PANEL A: 2D Minimap Overhead with Cross-Terminal Background Blocks
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

    // Draw the Player Avatar safely centered in their space slot
    char avatar = '^';
    switch (player.dir) {
        case DIR_NORTH: avatar = '^'; break;
        case DIR_EAST:  avatar = '>'; break;
        case DIR_SOUTH: avatar = 'v'; break;
        case DIR_WEST:  avatar = '<'; break;
    }
    int player_screen_x = left_panel_x + (player.pos.x * 2);
    mvaddch(player.pos.y, player_screen_x,     ' '    | COLOR_PAIR(COLOR_TEXT));
    mvaddch(player.pos.y, player_screen_x + 1, avatar | COLOR_PAIR(COLOR_TEXT));




    // PANEL B: The 3D Wireframe Frame Window Viewport Outer Border Box

    for (int y = 0; y < game_map.rows || y < VIEW_H + 4; y++) {
        mvaddch(y, right_panel_x - 3, '|');
    }

    mvprintw(0, right_panel_x, "--- 3D RAYCAST VIEWPORT ---");
    for (int x = 0; x < VIEW_W + 2; x++) {
        mvaddch(viewport_y - 1, right_panel_x - 1 + x, '-');
        mvaddch(viewport_y + VIEW_H, right_panel_x - 1 + x, '-');
    }
    for (int y = 0; y < VIEW_H; y++) {
        mvaddch(viewport_y + y, right_panel_x - 1, '|');
        mvaddch(viewport_y + y, right_panel_x + VIEW_W, '|');
    }

    // --- PAINTER'S ALGORITHM 3D RENDERING ENGINE LOOP ---

    // LAYER 3: Far Distance rendering (Depth 3)
    if (frustum.cells[3][1] == '#' || frustum.cells[3][1] == 'E') {
        draw_front_wall(right_panel_x, viewport_y, depth_rects[3]);
    }

    // LAYER 2: Mid Distance rendering (Depth 2)
    // Left side-wall segment
    if (frustum.cells[2][0] == '#') {
        draw_side_wall_lines(right_panel_x, viewport_y, depth_rects[2], depth_rects[3], 1, 0);
        // Draw vertical lines at both ends of this segment
        for (int y = depth_rects[3].y0; y <= depth_rects[3].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[3].x0, ACS_VLINE);
        for (int y = depth_rects[2].y0; y <= depth_rects[2].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[2].x0, ACS_VLINE);
    }
    // Right side-wall segment
    if (frustum.cells[2][2] == '#') {
        draw_side_wall_lines(right_panel_x, viewport_y, depth_rects[2], depth_rects[3], 0, 1);
        // Draw vertical lines at both ends of this segment
        for (int y = depth_rects[3].y0; y <= depth_rects[3].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[3].x1, ACS_VLINE);
        for (int y = depth_rects[2].y0; y <= depth_rects[2].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[2].x1, ACS_VLINE);
    }
    // Front face
    if (frustum.cells[2][1] == '#' || frustum.cells[2][1] == 'E') {
        clear_sub_rect(right_panel_x, viewport_y, depth_rects[2]);
        draw_front_wall(right_panel_x, viewport_y, depth_rects[2]);
    }

    // LAYER 1: Close Distance rendering (Depth 1)
    // Left side-wall segment
    if (frustum.cells[1][0] == '#') {
        draw_side_wall_lines(right_panel_x, viewport_y, depth_rects[1], depth_rects[2], 1, 0);
        // Draw vertical lines at both ends of this segment
        for (int y = depth_rects[2].y0; y <= depth_rects[2].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[2].x0, ACS_VLINE);
        for (int y = depth_rects[1].y0; y <= depth_rects[1].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[1].x0, ACS_VLINE);
    }
    // Right side-wall segment
    if (frustum.cells[1][2] == '#') {
        draw_side_wall_lines(right_panel_x, viewport_y, depth_rects[1], depth_rects[2], 0, 1);
        // Draw vertical lines at both ends of this segment
        for (int y = depth_rects[2].y0; y <= depth_rects[2].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[2].x1, ACS_VLINE);
        for (int y = depth_rects[1].y0; y <= depth_rects[1].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[1].x1, ACS_VLINE);
    }
    // Front face
    if (frustum.cells[1][1] == '#' || frustum.cells[1][1] == 'E') {
        clear_sub_rect(right_panel_x, viewport_y, depth_rects[1]);
        draw_front_wall(right_panel_x, viewport_y, depth_rects[1]);
    }

    // LAYER 0: Immediate walls adjacent to player sides
    // Left side-wall segment
    if (frustum.cells[0][0] == '#') {
        draw_side_wall_lines(right_panel_x, viewport_y, depth_rects[0], depth_rects[1], 1, 0);
        // Draw vertical lines at both ends of this segment
        for (int y = depth_rects[1].y0; y <= depth_rects[1].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[1].x0, ACS_VLINE);
        for (int y = depth_rects[0].y0; y <= depth_rects[0].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[0].x0, ACS_VLINE);
    }
    // Right side-wall segment
    if (frustum.cells[0][2] == '#') {
        draw_side_wall_lines(right_panel_x, viewport_y, depth_rects[0], depth_rects[1], 0, 1);
        // Draw vertical lines at both ends of this segment
        for (int y = depth_rects[1].y0; y <= depth_rects[1].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[1].x1, ACS_VLINE);
        for (int y = depth_rects[0].y0; y <= depth_rects[0].y1; y++) mvaddch(viewport_y + y, right_panel_x + depth_rects[0].x1, ACS_VLINE);
    }





    // Exit text alert inside view bounds
    if (frustum.cells[1][1] == 'E' || frustum.cells[2][1] == 'E' || frustum.cells[3][1] == 'E') {
        mvprintw(viewport_y + 10, right_panel_x + 15, "[ EXIT ]");
    }

    // Interface Info Footer
    int footer_y = (game_map.rows > VIEW_H + 4) ? game_map.rows : VIEW_H + 4;
    mvprintw(footer_y + 1, 0, "Controls: UP/DOWN (Walk), LEFT/RIGHT (Turn). Press 'q' to quit.");
    const char* dir_names[] = {"NORTH", "EAST", "SOUTH", "WEST"};
    mvprintw(footer_y + 2, 0, "Vector Core: X:%d Y:%d | Facing vector: %s", player.pos.x, player.pos.y, dir_names[player.dir]);

    refresh();
}

int main() {
    if (!load_map("map.txt")) {
        printf("Error: Could not open map.txt.\n");
        return 1;
    }
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    // Inside main() after initscr():
    if (has_colors()) {
        start_color();
        // Pair 1: Text color doesn't matter much since we are filling the background
        init_pair(COLOR_WALL, COLOR_BLACK, COLOR_WHITE); // White walls
        init_pair(COLOR_TEXT, COLOR_WHITE, COLOR_BLACK); // Normal text
    }

    while (game_running) {
        render_game();
        int ch = getch();
        switch (ch) {
            case KEY_UP:    move_player(1);   break;
            case KEY_DOWN:  move_player(-1);  break;
            case KEY_LEFT:  turn_player(-1);  break;
            case KEY_RIGHT: turn_player(1);   break;
            case 'q':       game_running = 0; break;
            default:        break;
        }
    }
    endwin();
    if (player_won) printf("\n🎉 Congratulations! You solved the maze completely from a 3D view!\n\n");
    else printf("\nEngine shutdown completed.\n\n");
    return 0;
}

