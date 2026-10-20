//  ansi_colors.c
//
//  Created by Rob Ross on 10/2/26.

/*

**ANSI Color Support and Modeling**

To evaluate color in terminal systems effectively, we must weigh the trade-offs between bandwidth efficiency, compatibility, and visual fidelity to maximize practical utility. ANSI color support has evolved through three primary tiers, each balancing computational resource costs against information display capacity.

---

**1. How Many Colors Are Supported?**

Modern terminal emulators adhering to ANSI escape sequence extensions support three main tiers:

- **16 Colors (4-bit)**: 8 standard colors (indices `0–7`) and 8 high-intensity/bright variants (indices `8–15`).
- **256 Colors (8-bit)**: An indexed palette composed of the initial 16 colors, a 216-color RGB cube (`16–231`), and a 24-step grayscale ramp (`232–255`).
- **16,777,216 Colors (24-bit "TrueColor")**: Direct 24-bit RGB representation ($256 \times 256 \times 256$).

---

**2. How ANSI Models Colors**

ANSI models color using escape sequences that instruct the terminal display engine how to interpret subsequent characters:

**A. Basic 16-Color Palette (Attribute-Based / 3-bit & 4-bit)**

- **Escape Syntax**: `\033[30m`–`\033[37m` (standard foreground), `\033[90m`–`\033[97m` (high-intensity foreground), and `\033[40m`–`\033[47m` / `\033[100m`–`\033[107m` (background).
- **Mechanism**: Translates directly to fixed logical color slots mapped to terminal-configured themes. This maximizes computational and bandwidth efficiency when only basic semantic distinctions (e.g., error red vs. success green) are needed.

**B. 256-Color Lookup Table (Indexed 8-bit)**

- **Escape Syntax**: `\033[38;5;<index>m` (foreground) or `\033[48;5;<index>m` (background), where `<index>` ranges from `0` to `255`.
- **Mechanism**: References an indexed palette structured as follows:
  - `0–15`: Standard and high-intensity system colors.
  - `16–231`: A $6 \times 6 \times 6$ RGB cube with 6 stepped intensity levels ($0, 1, 2, 3, 4, 5$) per channel ($6^3 = 216$ colors).
  - `232–255`: 24 grayscale increments from dark to light.
- **Utility Benefit**: Delivers a rich spectrum suitable for complex data visualizations while keeping escape sequences compact (only requiring a single byte index).

**C. Direct 24-Bit Color (TrueColor)**

- **Escape Syntax**: `\033[38;2;<r>;<g>;<b>m` (foreground) or `\033[48;2;<r>;<g>;<b>m` (background).
- **Mechanism**: Explicitly specifies red, green, and blue components directly to bypass palette lookups.

---

**3. Does It Use Standard RGB Values (256 Possible for Each)?**

- **In 24-bit TrueColor mode**: **Yes.** It uses full 8-bit depth per channel, allowing standard integer values from `0` to `255` for red, green, and blue channels ($256 \times 256 \times 256 = 16,777,216$ total values).
- **In 256-color indexed mode**: **No.** The 216-color portion of the 256-color palette samples only 6 discrete levels per channel (usually mapped by the terminal renderer to approximate 8-bit values like `0`, `95`, `135`, `175`, `215`, `255`), reducing channel granularity to optimize data transmission over constrained channels.

---

**Optimal Usage Consideration**

- For simple CLI status indicators and logs, using the basic 16-color set produces the greatest utility by ensuring maximum portability across legacy environments and minimal character overhead.
- For data dashboards, syntax highlighters, and terminal UIs, 256-color or 24-bit TrueColor maximizes information density and readability, providing the highest visual benefit for the user.
*/


#include <stdio.h>
#include <locale.h>

void standard();
void high_intensity();
void idk();
void grayscale();
void grayscale2();
void grayscale_red();
void grayscale_green();
void grayscale_blue();

/**
 * Run as `ansi_colors | less -SR`
 * -S (--chop-long-lines): Disables line wrapping and enables horizontal panning.
 * -R (--RAW-CONTROL-CHARS): Preserves ANSI/TrueColor escape sequences.
 *
 */
int main() {
    setlocale(LC_ALL, ""); // Initializes the system's native UTF-8 locale
    // Disable auto-wrap (DECAWM Reset)
    printf("\033[?7l");

#if 0
    standard();
    high_intensity();
    idk();
    grayscale();
    grayscale2();
# endif

    grayscale_red();
    grayscale_green();
    grayscale_blue();

    // Re-enable auto-wrap before exiting to preserve terminal state
    printf("\033[?7h");
    return 0;
}

void standard() {
    printf("Standard colors\n");

    for (int i = 0; i <= 7; i++) {
        printf("\033[48;5;%dm %3d\033[m", i, i);
    }
    printf("\n\n");
}

void high_intensity() {
    printf("High-intensity colors\n");

    for (int i = 8; i <= 15; i++) {
        printf("\033[48;5;%dm %3d\033[m", i, i);
    }
    printf("\n\n");
}

void idk() {
    printf("216 colors\n");

    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 36; col++) {
            int i = 36 * row + col + 16;
            printf("\033[48;5;%dm %3d ", i, i);
        }
        printf("\n");
    }
    printf("\n");
}

void grayscale() {
    printf("Grayscale colors\n");

    for (int i = 232; i <= 255; i++) {
        printf("\033[48;5;%dm %3d\033[m", i, i);
    }
    printf("\n\n");
}

void grayscale2() {
    printf("All Grayscale colors\n");
    // Make a display square. sqrt of 256 is 16. square is 16 x 16
    int entries_per_row = 16;
    for (int i = 0; i < 256; i ++) {
        if (i % 16 == 0) printf("\n"); // new row
        printf("\033[48;2;%d;%d;%dm %3d\033[m", i, i, i, i);
    }
    printf("\n\n");
}

// greyscale is r==g==b. Thus there are 256 possible grayscale colors.
// if we want to vary one of the r,g,b parameters from 1-256 while keeping the other two the same,
// that would give us 256*256==65,536 combinations. Too many to print.
// But fuck it, let's see what happens if we try! LOL ;p

void grayscale_red() {
    printf("Grayscale with red\n");


    for (int red = 0; red < 256; red++) {
        printf("r:%3d ", red);
        for (int gray = 0; gray < 256; gray++) {
            //                 r  g  b    g
            printf("\033[48;2;%d;%d;%dm %3d\033[m", red, gray, gray, gray); // r, g, b, grey
        }
        printf(" r:%3d ", red);
        printf("\n");
    }
    printf("\n");
}

void grayscale_green() {
    printf("Grayscale with green\n");

    for (int green = 0; green < 256; green++) {
        printf("g:%3d ", green);
        for (int gray = 0; gray < 256; gray++) {
            //                 r  g  b    g
            printf("\033[48;2;%d;%d;%dm %3d\033[m", gray, green, gray, gray); // r, g, b, grey
        }
        printf(" g:%3d ", green);
        printf("\n");
    }
    printf("\n");
}

void grayscale_blue() {
    printf("Grayscale with blue\n");

    for (int blue = 0; blue < 256; blue++) {
        printf("b:%3d ", blue);
        for (int gray = 0; gray < 256; gray++) {
            //                 r  g  b    g
            printf("\033[48;2;%d;%d;%dm %3d\033[m", gray, gray, blue, gray); // r, g, b, grey
        }
        printf(" b:%3d ", blue);
        printf("\n");
    }
    printf("\n");
}
