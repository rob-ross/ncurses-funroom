//  old_map_file_converter.c
//
//  Created by Rob Ross on 10/30/26.
//
//  Copyright (c) 2026.  All rights reserved.
//

// converts the old version of an interim encoding scheme for maps in my ncurses maze exploration.
// old format encoded walls with a 4 bit flag field with the layout
// W N S E
// to indicate the presence or absence of a wall to the West, North, South, or East of the map cell.
// To be consistent with enums and make it easy to translate the flags
// to relative orientations I changed the encoding to:
// W S E N
/*

typedef enum : unsigned char {
    WF_NORTH = 1u << 0, // bit 0 (1)
    WF_EAST  = 1u << 1, // bit 1 (2)
    WF_SOUTH = 1u << 2, // bit 2 (4)
    WF_WEST  = 1u << 3, // bit 3 (8)
} WallFlags;
*/

// This program converts files in the old encoding to the new encoding.

// compile with : clang -O2 old_map_file_converter.c -o convert_map
// run like: ./convert_map map3.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Converts a single character:
 * - If hex (0-9, A-F, a-f): converts from W-N-S-E to N-E-S-W bit encoding.
 * - Otherwise (newlines, spaces, etc.): returns the character unchanged.
 */
char convert_hex_char(char c) {
    int val = -1;
    if (c >= '0' && c <= '9') {
        val = c - '0';
    } else if (c >= 'a' && c <= 'f') {
        val = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        val = c - 'A' + 10;
    }

    if (val < 0) {
        return c; // Keep newlines, formatting, etc.
    }

    // Old Bit Layout:
    // bit 3 = West, bit 2 = North, bit 1 = South, bit 0 = East
    int old_w = (val >> 3) & 1;
    int old_n = (val >> 2) & 1;
    int old_s = (val >> 1) & 1;
    int old_e = (val >> 0) & 1;

    // New Clockwise Bit Layout:
    // bit 0 = North, bit 1 = East, bit 2 = South, bit 3 = West
    int new_val = (old_n << 0) | (old_e << 1) | (old_s << 2) | (old_w << 3);

    // Format back to hex char (uppercase)
    if (new_val < 10) {
        return (char)('0' + new_val);
    } else {
        return (char)('A' + (new_val - 10));
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <map_filename>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];

    // 1. Open the input file
    FILE *in = fopen(input_path, "r");
    if (!in) {
        perror("Error opening input file");
        return 1;
    }

    // Extract the base filename (strips preceding directory paths if any)
    const char *base_filename = strrchr(input_path, '/');
#ifdef _WIN32
    const char *base_win = strrchr(input_path, '\\');
    if (base_win && (!base_filename || base_win > base_filename)) {
        base_filename = base_win;
    }
#endif
    base_filename = (base_filename != NULL) ? base_filename + 1 : input_path;

    // 2 & 4. Build output filename in current directory: "converted_<base_filename>"
    char output_filename[512];
    snprintf(output_filename, sizeof(output_filename), "converted_%s", base_filename);

    FILE *out = fopen(output_filename, "w");
    if (!out) {
        perror("Error creating output file");
        fclose(in);
        return 1;
    }

    // 3. Process each character
    int ch;
    while ((ch = fgetc(in)) != EOF) {
        fputc(convert_hex_char((char)ch), out);
    }

    fclose(in);
    fclose(out);

    printf("Successfully converted '%s' -> '%s'\n", input_path, output_filename);
    return 0;
}
