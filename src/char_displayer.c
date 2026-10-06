//  char_displayer.c
//
//  Created by Rob Ross on 10/3/26.
//
//  Copyright (c) 2026.  All rights reserved.
//

// Official Unicode Metadata: The Unicode Character Database (UCD)
// simple terminal UI to browser unicode glyphs
// Official URL: https://www.unicode.org/Public/UCD/latest/ucd/Blocks.txt
// Format: <start_hex>..<end_hex>; <Block Name

//4. Official Documentation & Interactive Lookup

// UCD Index & Documentation: https://www.unicode.org/ucd/
// Standard Annex #44 (UCD Specification): https://www.unicode.org/reports/tr44/
// Official Character Code Charts (PDFs): https://www.unicode.org/charts/ (includes visual glyph charts for Block Elements and Geometric Shapes).

/*

Blocks.txt
https://www.unicode.org/Public/UCD/latest/ucd/Blocks.txt
Code point ranges and official names for all character blocks

UnicodeData.txt
https://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt
Character-by-character metadata: official names (e.g. BLACK SQUARE), general category (So, Sm, etc.), bidirectional class, and case mappings

NamesList.txt
https://www.unicode.org/Public/UCD/latest/ucd/NamesList.txt
Human-readable descriptions, cross-references (=, x), sub-headers, and character annotations

EastAsianWidth.txt
https://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt
Terminal/CJK column width properties (W for Wide, N for Narrow, A for Ambiguous)
+-----------------------+-------------------------+--------------------+
| Property Code         | Formal Name             | Terminal Width     |
+-----------------------+-------------------------+--------------------+
| F                     | Fullwidth               | 2 columns          |
| H                     | Halfwidth               | 1 column           |
| W                     | Wide                    | 2 columns          |
| Na                    | Narrow                  | 1 column           |
| N                     | Neutral                 | 1 column (default) |
| A                     | Ambiguous               | 1 or 2 columns     |
+-----------------------+-------------------------+--------------------+

emoji-data.txt
https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt
Emoji classifications, presentation defaults, and variation sequences
 */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <wchar.h>

#define MAX_SETS 400
constexpr size_t MAX_SET_NAME_SIZE = 255;

typedef struct unichar_set_t {
    unsigned start;
    unsigned end;
    unsigned size;
    char name[MAX_SET_NAME_SIZE + 1]; // set names have max 255 chars
}  UniCharSet;

enum uchar_width_e {
    UWIDTH_RANGE, UWIDTH_SINGLE
};

enum uchar_width_code_e {
    UWIDTH_FULL, UWIDTH_HALF, UWIDTH_WIDE, UWIDTH_NARROW, UWIDTH_NEUTRAL, UWIDTH_AMBIGUOUS
};

typedef struct unichar_width_t {
    enum uchar_width_e width_entry_type;
    enum uchar_width_code_e width_code;
    unsigned start; // for ranges and singles
    unsigned end;   // unused for singles
    int width;
} UniCharWidth;

typedef struct unichar_width_adjust_t {
    unsigned codepoint;
    int      adjustment;
} UniCharWidthAdjustments;

// todo to do this correct, we need to allocate heap space for this structure so it can handle any data in the future
constexpr size_t MAX_WIDTH_ENTRIES = 3000; // more than generous based on data in EastAsianWidth.txt file.
static UniCharWidth uchar_widths[MAX_WIDTH_ENTRIES];
static unsigned num_uchar_widths = {0};

typedef struct unichar_width_adjust_t {
    unsigned codepoint;
    int      adjustment;
} UniCharWidthAdjustments;

constexpr size_t MAX_WIDTH_ADJUSTMENTS = 100; // todo temp
static UniCharWidthAdjustments uchar_adjustments[MAX_WIDTH_ADJUSTMENTS] = {
    {0x0149, -1},
    {0x01C4, -1},
    {0x0489, -1},

};
static unsigned num_uchar_adjustments = 2;


// Forward declarations
static unsigned width_for_codepoint(unsigned codepoint);
static void print_widths(unsigned codepoint);
static unsigned width_adjustment(unsigned codepoint);

static int load_east_asian_widths(const char * filename, UniCharWidth uwidths[], size_t max_entries ) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error trying to open '%s': ", filename);
        perror("");
        return -1;
    }
    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < max_entries) {
        // Skip comment lines and blank lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        char width_code[3]; // 2 chars max ('Na') + 1 for null terminator '\0'
        unsigned start = 0;
        unsigned end = 0;


        // Format:
        // 0000..001F     ; N  # ...
        // or
        // 0020           ; Na # ...
        // Try range format first: "0021..0023     ; Na ..."
        if (sscanf(line, "%x..%x ; %2s", &start, &end, width_code) == 3 ||
            sscanf(line, "%x ; %2s", &start, width_code) == 2)
        {
                if (sscanf(line, "%x..%x ; %2s", &start, &end, width_code) == 3) {
                    uwidths[count].width_entry_type = UWIDTH_RANGE;
                    uwidths[count].start = start;
                    uwidths[count].end = end;
                }
                // Fall back to single codepoint format: "0024           ; Na ..."
                else if (sscanf(line, "%x ; %2s", &start, width_code) == 2) {
                    uwidths[count].width_entry_type = UWIDTH_SINGLE;
                    uwidths[count].start = start;
                    uwidths[count].end = start; // Setting end = start simplifies downstream range searches
                }
                uwidths[count].width = 1;
                // Map width_code (e.g., "Na", "W", "A", etc.) to width properties
                // this translates "Na" (Narrow) as just N (Neutral); both are width 1
                switch (width_code[0]) {
                    case 'F':
                        uwidths[count].width_code = UWIDTH_FULL;
                        uwidths[count].width = 2;
                        break;
                    case 'W':
                        uwidths[count].width_code = UWIDTH_WIDE;
                        uwidths[count].width = 2;
                        break;
                    case 'H':
                        uwidths[count].width_code = UWIDTH_HALF;
                        uwidths[count].width = 1;
                        break;
                    case 'N': // Narrow or Neutral
                        if (width_code[1] == 'a') {
                            uwidths[count].width_code = UWIDTH_NARROW;
                            uwidths[count].width = 1;
                        } else {
                            uwidths[count].width_code = UWIDTH_NEUTRAL;
                            uwidths[count].width = 1; // todo see if this makes spacing better
                        }
                        break;
                    case 'A': // ambiguous
                        uwidths[count].width_code = UWIDTH_AMBIGUOUS;
                        uwidths[count].width = 1;  // here we default to 2 for now. Setting is also controlled in console
                        break;
                }
                count++;

        }
    }

END:
    fclose(fp);
    printf("load_east_asian_widths loaded %d ranges.\n", count);
    return count;
}

UniCharSet unichar_sets[MAX_SETS];
// {
//     { .start = 0x25A0, .end = 0x25FF, .size = 0x25FF - 0x25A0 + 1, .name = "Geometric Shapes" },
//     { .start = 0x2580, .end = 0x259F, .size = 0x259F - 0x2580 + 1, .name = "Block Elements"},
//     //0250..02AF; IPA Extensions
//     { .start = 0x0250, .end = 0x02AF, .size = 0x02AF - 0x0250 + 1, .name = "IPA Extensions"},
// };

int load_unicode_blocks(const char *filename, UniCharSet sets[], int max_sets) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error trying to open '%s': ", filename);
        perror("");
        return -1;
    }

    char line[512];
    int count = 0;

    while (fgets(line, sizeof(line), fp) && count < max_sets) {
        // Skip comment lines and blank lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        unsigned start, end;
        char name[MAX_SET_NAME_SIZE + 1];

        // Format: "25A0..25FF; Geometric Shapes"
        if (sscanf(line, "%x..%x; %255[^\r\n]", &start, &end, name) == 3) {
            if (start >= 0xFFFF) {
                // We'll stick to the BMP for now.
                goto END;
            }
            if ( (start >= 0xD800 && start <= 0xDFFF) || ( end >= 0xD800 && end <=0xDFFF )) {
                // surrogate pairs, skip
                continue;
            }

            sets[count].start = start;
            sets[count].end = end;
            sets[count].size = end - start + 1;
            if (sets[count].size > 256) continue; // skip large blocks
            strncpy(sets[count].name, name, MAX_SET_NAME_SIZE);
            sets[count].name[MAX_SET_NAME_SIZE] = '\0';
            count++;
        }
    }
END:
    fclose(fp);
    return count;
}

// For a block, find the max number of spaces (width) required for any character in the block
// Returns the maximum column width (0, 1, or 2) required for printable characters in a block
unsigned get_max_codepoint_width(UniCharSet block) {
    int max_cols = 0;

    for (unsigned codepoint = block.start; codepoint <= block.end; codepoint++) {
        int width = wcwidth((wchar_t)codepoint);

        // Only consider valid printable characters (width >= 0)
        if (width > max_cols) {
            max_cols = width;
        }
    }

    // Default to at least 1 column if all characters were non-printable or 0-width
    return (max_cols > 0) ? (unsigned)max_cols : 1;
}

void encode_unicode_to_utf8(unsigned codepoint, char buf[5]) {
    if (codepoint <= 0x7F) {
        // 1-byte sequence (ASCII: 0x00 - 0x7F)
        buf[0] = (char)codepoint;
        buf[1] = '\0';
    }
    else if (codepoint <= 0x7FF) {
        // 2-byte sequence (0x80 - 0x7FF)
        buf[0] = (char)(0xC0 | ((codepoint >> 6) & 0x1F));
        buf[1] = (char)(0x80 | (codepoint & 0x3F));
        buf[2] = '\0';
    }
    else if (codepoint <= 0xFFFF) {
        // 3-byte sequence (0x800 - 0xFFFF)
        // Note: 0xD800 - 0xDFFF are reserved UTF-16 surrogates and are invalid in UTF-8
        if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
            buf[0] = '\0'; // Invalid surrogate codepoint
            return;
        }
        buf[0] = (char)(0xE0 | ((codepoint >> 12) & 0x0F));
        buf[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (codepoint & 0x3F));
        buf[3] = '\0';
    }
    else if (codepoint <= 0x10FFFF) {
        // 4-byte sequence (0x10000 - 0x10FFFF)
        buf[0] = (char)(0xF0 | ((codepoint >> 18) & 0x07));
        buf[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buf[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buf[3] = (char)(0x80 | (codepoint & 0x3F));
        buf[4] = '\0';
    }
    else {
        // Invalid code point (> U+10FFFF)
        buf[0] = '\0';
    }
}

int manual_width_adjustment( unsigned codepoint) {
    for (int i = 0; i < num_uchar_adjustments; i++) {
        if ( uchar_adjustments[i].codepoint == codepoint) {
            return uchar_adjustments[i].adjustment;
        }
    }
    return 0;
}

static unsigned width_adjustment(unsigned codepoint) {
    int wcw = wcwidth((wchar_t)codepoint);  // if returns -1, no character defined for that codepoint
    unsigned wcp = width_for_codepoint(codepoint);

    if (wcw == -1 ) return 0;

    // if ( wcw == -1 || wcw == 0 ) return 0;

    if (wcw == -1 && wcp == 0 ) {
        return 0;
    }

    if (wcw == -1 && wcp == 1) return 1;

    if ( wcw == -1 && wcp == 0 ) {
        // return 0;
        // // wcwidth has no value for the width. Happens with non-printing characters
        // fprintf(stderr,"WARNING, wcwidth returns -1 for %.4X\n", codepoint);
        // print_widths(codepoint);
        // // wcw = 1;
    }

    if ( wcw == 0 ) {
        // adjustment needs to add more spaces
        return wcp - wcw;

    }

    if (wcw == wcp) return 0;
    return wcw - wcp;
    //
    // // main expected use case
    // if (wcw > wcp) {
    //     return wcw - wcp;
    // }
    //
    //
    //
    // if (wcp > wcw) {
    //     // fprintf(stderr, "WARNING, wcp > wcw for %.4X\n", codepoint);
    //     // print_widths(codepoint);
    //     return 0;
    // }
    //
    //
    //
    // // fprintf(stderr, "WARNING, width_adjustment not returning value for %.4X\n", codepoint);
    // return 0;



}


void print_UniCharSet(UniCharSet set) {
    printf("%s\n", set.name);
    // print column headers
    // ---------------------
    // space for row header, allows for 4 spaces plus colon.
    printf("     "); // todo , use max width needed from largest code point. <=BMP range == 5 spaces, max extended
    // plane up to 7 spaces needed

    for (unsigned i = 0; i <  16; i++) {
        printf("%4.2X", i );
        // for (int i = 0; i <= ns; i++) {
        //     printf(" ");
        // }
    }
    putchar('\n');

    // print row glyphs
    // ---------------------
    int width = 4; // start with default 4 space width per glyph.
    if (set.start >= 0x10000) {
        width = width + 3; // 4 bytes per character
    } else if (set.start >=0x0800) {
        width = width + 2; // 3 bytes per character
    } else if (set.start >= 0x80) {
        width = width + 1; // these characters are encoded with 2 bytes.
    }
    // printf("width based on bytes: width = %u\n", width );

    char buffer[5] = {};
    for (unsigned row_start = set.start; row_start < (set.start + set.size); row_start += 16) {
        printf("%.4X:", row_start); // row header
        for (unsigned codepoint = row_start; codepoint < row_start + 16; codepoint++) {
            if (codepoint < 0x20) {
                // non printing control character.
                // char cntrl[4]= {};
                // snprintf(cntrl, 4, "^%X", codepoint );
                // printf("%*s", width, cntrl);
                printf("%*s", width, "");

            } else {
                encode_unicode_to_utf8(codepoint, buffer);

                //print hex digits for now, strings aren't coming out
                // for ( char *p = buffer; *p; p++) {
                //     printf("%2X", (unsigned char)*p);
                // }
                // printf(" ");
                // Appending \uFE0E forces standard text presentation
                // my terminal doesn't seem to support this escape
                // printf("%3s\xEF\xB8\x8E  ", buffer);

                // int width_adjust = width_for_codepoint(codepoint) == 2 ? -1 : 0;
                if (codepoint == 0x25FD || codepoint==0x25FE) {
                    // printf("%X: width=%d, width_adjust=%d, width_for_codepoint=%d,", codepoint, width, width_adjust, width_for_codepoint(codepoint));
                }
                printf("%*s", width + width_adjustment(codepoint) + manual_width_adjustment(codepoint), buffer);
            }

        }
        putchar('\n');
    }
}

// returns col width for the given codepoint as defined in EastAsianWidths.txt.
// Ambiguous widths return 1
// uses binary search, O(log n) lookup
static unsigned width_for_codepoint(unsigned codepoint) {
    unsigned return_width = -1;
    if (codepoint > 0x10FFFF) {
        return return_width;
    }

    int left = 0;
    int right = (int)num_uchar_widths - 1;
    bool     found = false;
    int middle = (right + left) / 2;

    while (left <= right && !found) {
        auto elem = uchar_widths[middle];
        if ( elem.start == codepoint ||
            (elem.width_entry_type == UWIDTH_RANGE && elem.start < codepoint && elem.end >= codepoint)) {
            found = true;
            return_width = elem.width;
            continue;
        } else if ( codepoint < elem.start) {
            right = middle - 1 ? middle - 1 : 0  ;
        } else {
            left = (middle + 1 < num_uchar_widths) ? middle + 1 : (int)num_uchar_widths - 1 ;
        }
        middle = (right + left) / 2;
    }


    // standard binary search of sorted array uchar_widths
    return return_width;
}

void print_widths(unsigned codepoint) {
    printf("wcwidth for 0x%.4X: %d, ", codepoint, wcwidth((wchar_t)codepoint));
    printf("width_for_codepoint for 0x%.4X: %d\n", codepoint, width_for_codepoint(codepoint));

}

int main() {
    setlocale(LC_ALL, ""); // Initializes the system's native UTF-8 locale
    int num_blocks = load_unicode_blocks("./Blocks.txt", unichar_sets, MAX_SETS);
    if (num_blocks == -1) exit(-1);
    num_uchar_widths = load_east_asian_widths("./EastAsianWidth.txt", uchar_widths, MAX_WIDTH_ENTRIES);
    if (num_uchar_widths == -1) exit (-1);


    for (int i = 0; i < num_blocks; ++i) {
        print_UniCharSet(unichar_sets[i]);
        putchar('\n');
    }



    // debug print ranges to verify widths
    /*
    for (unsigned i = 0; i < num_uchar_widths; i++ ) {
        auto elem = uchar_widths[i];
        if (elem.start < 0x2500 || elem.start > 0x2600) continue;
        printf("start:%X, end:%X, width:%d, code:%c\n", elem.start, elem.end, elem.width, elem.width_char);
    }
    */

    printf("\nDisplayed %d blocks.\n", num_blocks);

    // printf("wcwidth for 0x25FD: %d\n", wcwidth((wchar_t)0x25FD));
    // printf("wcwidth for 0x25FE: %d\n", wcwidth((wchar_t)0x25FE));
    // printf("wcwidth for 0x0149: %d\n", wcwidth((wchar_t)0x0149));
    // printf("width_for_codepoint for 0x25FD: %d\n", width_for_codepoint(0x25FD));
    // printf("width_for_codepoint for 0x25FE: %d\n", width_for_codepoint(0x25FE));
    // printf("width_for_codepoint for 0x0149: %d\n", width_for_codepoint(0x0149));

    // for (unsigned cp = 0x0300; cp <= 0x030F; cp++) {
    //     print_widths(cp);
    // }

    // for (unsigned cp = 0x0370; cp <= 0x037F; cp++) {
    //     print_widths(cp);
    // }

    // for (unsigned cp = 0x00C0; cp <= 0x00CF; cp++) {
    //     print_widths(cp);
    // }

    // for (unsigned cp = 0x02F0; cp <= 0x02FF; cp++) {
    //     print_widths(cp);
    // }

    for (unsigned cp = 0x0590; cp <= 0x059F; cp++) {
        print_widths(cp);
    }

}
