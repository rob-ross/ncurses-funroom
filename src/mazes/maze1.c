//  maze1.c
//
//  Created by Rob Ross on 10/27/26.
//
//  Copyright (c) 2026.  All rights reserved.

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
    Coord pos;
} Player;

typedef struct {
    char grid[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
} Map;

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

// Process player movement and collision checks
void try_move(int dx, int dy) {
    Coord target = { player.pos.x + dx, player.pos.y + dy };
    char tile = get_tile_at(target);

    if (tile == ' ') {
        player.pos = target;
    } else if (tile == '#') {
        printw("\x07"); // print bell , doesn't do anything
        // instead, try to draw player in reverse
        move(target.y + dy, target.x+dx );
        addch('#' | A_REVERSE);
        refresh();
    } else if (tile == 'E') {
        player.pos = target;
        player_won = 1;
        game_running = 0;
    }
    // If it's '#', do nothing (collision block)
}

// Render the current frame
void render_game() {
    erase(); // ncurses clear screen buffer

    // Draw the static map layout
    for (int y = 0; y < game_map.rows; y++) {
        for (int x = 0; x < game_map.cols; x++) {
            mvaddch(y, x, game_map.grid[y][x]);
        }
    }

    // Overwrite with the dynamic player avatar
    mvaddch(player.pos.y, player.pos.x, 'P');

    // Print operational UI / controls status below the map
    mvprintw(game_map.rows + 1, 0, "Use Arrow Keys to navigate. Press 'q' to quit.");

    refresh(); // Push memory buffer to terminal screen
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
    render_game();
    while (game_running) {
        render_game();
        int ch = getch(); // Block and wait for a keystroke
        switch (ch) {
            case KEY_UP:    try_move(0, -1); break;
            case KEY_DOWN:  try_move(0, 1);  break;
            case KEY_LEFT:  try_move(-1, 0); break;
            case KEY_RIGHT: try_move(1, 0);  break;
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

