//  maze2.c
//
//  Created by Rob Ross on 10/27/26.
//
//  Copyright (c) 2026.  All rights reserved.

// modified maze1.c to take into account the player's orientation, N, E, S, W.
// up/down arrow now move "forward/backward", and left/right arrow change orientation


#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <wchar.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ROWS 100
#define MAX_COLS 100

// Structural abstractions to keep 3D upgrades easy later
typedef struct {
    int x;
    int y;
} Coord;



typedef struct {
    char grid[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
} Map;

typedef enum {
    DIR_NORTH = 0,
    DIR_EAST  = 1,
    DIR_SOUTH = 2,
    DIR_WEST  = 3
} Direction;

typedef struct {
    Coord pos;
    Direction dir; // Tracks which way the player is looking
} Player;



// Global Game State
Map game_map;
Player player;
int game_running = 1;
int player_won = 0;

// Load the map from an ASCII text file
int load_map(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 0;
    }

    game_map.rows = 0;
    game_map.cols = 0;

    char line[MAX_COLS + 2]; // Extra space for newline and null terminator
    while (fgets(line, sizeof(line), file) && game_map.rows < MAX_ROWS) {
        int c = 0;
        while (line[c] != '\n' && line[c] != '\0' && c < MAX_COLS) {
            game_map.grid[game_map.rows][c] = line[c];

            // Check for spawn point
            if (line[c] == 'P') {
                player.pos.x = c;
                player.pos.y = game_map.rows;
                game_map.grid[game_map.rows][c] = ' '; // Clear start cell to a walkable path
            }
            c++;
        }
        if (c > game_map.cols) {
            game_map.cols = c;
        }
        game_map.rows++;
    }

    fclose(file);
    return 1;
}

// Map lookup abstraction
char get_tile_at(Coord coord) {
    if (coord.x < 0 || coord.x >= game_map.cols || coord.y < 0 || coord.y >= game_map.rows) {
        return '#'; // Out of bounds behaves like a wall
    }
    return game_map.grid[coord.y][coord.x];
}

// Turn the player 90 degrees clockwise or counter-clockwise
void turn_player(int rotation) {
    // Adding 4 prevents negative numbers before the modulo operator handles looping
    player.dir = (player.dir + rotation + 4) % 4;
}

// Move forward or backward relative to current orientation
void move_player(int step) {
    int dx = 0;
    int dy = 0;

    switch (player.dir) {
        case DIR_NORTH: dy = -1 * step; break;
        case DIR_SOUTH: dy =  1 * step; break;
        case DIR_EAST:  dx =  1 * step; break;
        case DIR_WEST:  dx = -1 * step; break;
        default: break; //no op
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


void render_game() {
    clear();

    // Draw map layout
    for (int y = 0; y < game_map.rows; y++) {
        for (int x = 0; x < game_map.cols; x++) {
            mvaddch(y, x, game_map.grid[y][x]);
        }
    }

    // Determine character avatar based on current orientation direction
    char avatar = '^';
    switch (player.dir) {
        case DIR_NORTH: avatar = '^'; break;
        case DIR_EAST:  avatar = '>'; break;
        case DIR_SOUTH: avatar = 'v'; break;
        case DIR_WEST:  avatar = '<'; break;
    }

    // Draw the active player avatar
    mvaddch(player.pos.y, player.pos.x, avatar);

    // Print text status updates
    mvprintw(game_map.rows + 1, 0, "Controls: UP (Forward), DOWN (Backward), LEFT/RIGHT (Turn). Press 'q' to quit.");

    // Debug footer text tracking orientation state explicitly
    const char* dir_names[] = {"NORTH", "EAST", "SOUTH", "WEST"};
    mvprintw(game_map.rows + 2, 0, "Player Vector: X:%d Y:%d | Facing: %s", player.pos.x, player.pos.y, dir_names[player.dir]);

    refresh();
}


int main() {
    setlocale(LC_ALL, ""); // Initializes the system's native UTF-8 locale

    // 1. Initialize data
    if (!load_map("map.txt")) {
        wprintf((wchar_t*)L"Error: Could not open map.txt. Make sure it exists in the execution folder.\n");
        return 1;
    }

    // 2. Initialize ncurses system settings
    initscr();             // Start ncurses mode
    cbreak();              // Line buffering disabled, pass on everything
    noecho();              // Don't echo characters typed by user back to screen
    keypad(stdscr, TRUE);  // Enable arrow keys configuration capture
    curs_set(0);           // Hide physical terminal cursor blink

    // 3. Central Game Engine Loop
    while (game_running) {
        render_game();

        int ch = getch();
        switch (ch) {
            case KEY_UP:    move_player(1);   break; // Forward
            case KEY_DOWN:  move_player(-1);  break; // Backward
            case KEY_LEFT:  turn_player(-1);  break; // Turn counter-clockwise
            case KEY_RIGHT: turn_player(1);   break; // Turn clockwise
            case 'q':       game_running = 0; break;
            default:        break;
        }
    }

    move(game_map.rows + 1, 0);
    if (player_won) {
        printw("\n🎉 Congratulations! You reached the exit and escaped the maze!\n\n");
    } else {
        printw("\nGame exited safely.\n\n");
    }
    printw("Press any key to exit...");
    getch();

    // 4. Teardown / Post-Game Processing
    endwin(); // Restore terminal settings safely before exiting

    if (player_won) {
        wprintf((wchar_t*)L"\n🎉 Congratulations! You reached the exit and escaped the maze!\n\n");
    } else {
        wprintf((wchar_t*)L"\nGame exited safely.\n\n");
    }
    return 0;
}

