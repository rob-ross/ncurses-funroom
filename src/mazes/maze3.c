//  maze3.c
//
//  Created by Rob Ross on 10/28/26.
//
//  Copyright (c) 2026.  All rights reserved.

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ROWS 100
#define MAX_COLS 100

typedef enum {
    DIR_NORTH = 0,
    DIR_EAST  = 1,
    DIR_SOUTH = 2,
    DIR_WEST  = 3
} Direction;

typedef struct {
    int x;
    int y;
} Coord;

typedef struct {
    Coord pos;
    Direction dir;
} Player;

typedef struct {
    char grid[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
} Map;

// Frustum Grid Representation
// Index mappings: Depth (0 to 3 ahead), Offset (0=Left, 1=Center, 2=Right)
// Note: Row 0 only tracks Left (0) and Right (2) because Center is the player cell.
typedef struct {
    char cells[4][3];
} ViewFrustum;

Map game_map;
Player player;
int game_running = 1;
int player_won = 0;

int load_map(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;

    game_map.rows = 0;
    game_map.cols = 0;

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

char get_tile_at(Coord coord) {
    if (coord.x < 0 || coord.x >= game_map.cols || coord.y < 0 || coord.y >= game_map.rows) {
        return '#';
    }
    return game_map.grid[coord.y][coord.x];
}

// STEP 1: Compute Directional Vectors & Fill Frustum Matrix
ViewFrustum update_frustum() {
    ViewFrustum f;
    int fx = 0, fy = 0; // Forward unit vectors
    int rx = 0, ry = 0; // Right unit vectors

    switch (player.dir) {
        case DIR_NORTH: fx =  0; fy = -1; rx =  1; ry =  0; break;
        case DIR_EAST:  fx =  1; fy =  0; rx =  0; ry =  1; break;
        case DIR_SOUTH: fx =  0; fy =  1; rx = -1; ry =  0; break;
        case DIR_WEST:  fx = -1; fy =  0; rx =  0; ry = -1; break;
    }

    // Loop through 4 depth steps ahead (0 = current row, 3 = three steps ahead)
    for (int depth = 0; depth <= 3; depth++) {
        // Loop through left (-1), center (0), right (1) offsets
        for (int offset = -1; offset <= 1; offset++) {
            Coord target = {
                player.pos.x + (fx * depth) + (rx * offset),
                player.pos.y + (fy * depth) + (ry * offset)
            };

            // Map offset index (-1, 0, 1) to array index (0, 1, 2)
            f.cells[depth][offset + 1] = get_tile_at(target);
        }
    }
    return f;
}

void turn_player(int rotation) {
    player.dir = (player.dir + rotation + 4) % 4;
}

void move_player(int step) {
    int dx = 0, dy = 0;
    switch (player.dir) {
        case DIR_NORTH: dy = -step; break;
        case DIR_SOUTH: dy =  step; break;
        case DIR_EAST:  dx =  step; break;
        case DIR_WEST:  dx = -step; break;
    }

    Coord target = { player.pos.x + dx, player.pos.y + dy };
    char tile = get_tile_at(target);

    if (tile == ' ') {
        player.pos = target;
    } else if (tile == 'E') {
        player.pos = target;
        player_won = 1;
        game_running = 0;
    }
}

// STEP 2: Render Game Layout into Split Panel Viewports
void render_game() {
    clear();

    // Update Frustum tracking before writing buffers
    ViewFrustum frustum = update_frustum();

    // PANELS: Left Margin Config
    int left_panel_start_x = 0;
    int right_panel_start_x = game_map.cols + 6;

    // PANEL A: Draw Left Side Overhead View
    for (int y = 0; y < game_map.rows; y++) {
        for (int x = 0; x < game_map.cols; x++) {
            mvaddch(y, left_panel_start_x + x, game_map.grid[y][x]);
        }
    }
    char avatar = '^';
    switch (player.dir) {
        case DIR_NORTH: avatar = '^'; break;
        case DIR_EAST:  avatar = '>'; break;
        case DIR_SOUTH: avatar = 'v'; break;
        case DIR_WEST:  avatar = '<'; break;
    }
    mvaddch(player.pos.y, left_panel_start_x + player.pos.x, avatar);

    // SEPARATOR LINE
    for (int y = 0; y < game_map.rows; y++) {
        mvaddch(y, right_panel_start_x - 3, '|');
    }

    // PANEL B: Draw Right Side Viewport Window
    mvprintw(0, right_panel_start_x, "--- FIRST PERSON VIEWPORT ---");

    // Visually print our calculated 11-cell Frustum matrix layout
    mvprintw(2, right_panel_start_x, "Far Ahead  [Row 3]:  [%c] [%c] [%c]", frustum.cells[3][0], frustum.cells[3][1], frustum.cells[3][2]);
    mvprintw(3, right_panel_start_x, "Mid Ahead  [Row 2]:  [%c] [%c] [%c]", frustum.cells[2][0], frustum.cells[2][1], frustum.cells[2][2]);
    mvprintw(4, right_panel_start_x, "Near Ahead [Row 1]:  [%c] [%c] [%c]", frustum.cells[1][0], frustum.cells[1][1], frustum.cells[1][2]);
    mvprintw(5, right_panel_start_x, "Immed Side [Row 0]:  [%c] [P] [%c]", frustum.cells[0][0], frustum.cells[0][2]);

    // Footer Info
    mvprintw(game_map.rows + 1, 0, "Controls: UP/DOWN (Move), LEFT/RIGHT (Turn). Press 'q' to quit.");
    const char* dir_names[] = {"NORTH", "EAST", "SOUTH", "WEST"};
    mvprintw(game_map.rows + 2, 0, "Vector: X:%d Y:%d | Facing: %s", player.pos.x, player.pos.y, dir_names[player.dir]);

    refresh();
}

int main() {
    if (!load_map("map.txt")) {
        printf("Error: Could not open map.txt.\n");
        return 1;
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

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

    if (player_won) {
        printf("\n🎉 Congratulations! You reached the exit and escaped the maze!\n\n");
    } else {
        printf("\nGame exited safely.\n\n");
    }

    return 0;
}

