//  manual_term_escapes.c
//
//  Created by Rob Ross on 10/3/26.
//
//  Copyright (c) 2026.  All rights reserved.



/*
An SGR sequence follows the CSI (Control Sequence Introducer) format:
\033[  + {parameters} + \033[m
Where:
 \033[ is the CSI (ESC [).
 The numbers separated by semicolons are the parameters (e.g., 1 for bold, 31 for red foreground, 48;5;... for extended colors).
 m is the final command byte instructing the terminal parser: "Apply these parameters to the Graphic Rendition state."
Common SGR Examples:
 \033[0m — Reset all rendition attributes to default
 \033[1m — Bold / increased intensity
 \033[3m — Italic
 \033[4m — Underline
 \033[5m — Slow blink
 \033[6m — Rapid blink
 \033[31m — Red foreground text
 \033[42m — Green backgroun
 */

/*

**Interesting Legacy & DEC Escape Sequences**

Beyond the double-width and double-height line attributes (`\033#6`, `\033#3`, `\033#4`), VT100 and subsequent DEC terminals (VT102, VT220) introduced a rich variety of control sequences. Many of these are still supported by modern terminal emulators like Terminal.app, iTerm2, and xterm.

---

**1. `\033#8` — Screen Alignment Pattern (DECALN)**

```c
printf("\033#8");
```

- **What it does:** Instantly fills the entire terminal screen from top to bottom with capital `E` characters (`EEEE...`).
- **History:** Created for hardware technicians and factory workers in the 1970s. When calibrating CRT (cathode ray tube) monitors, an entire screen of `E`s was used to adjust focus, beam convergence, screen geometry, and aspect distortion.
- **Support:** Supported by almost all modern terminals (macOS Terminal, iTerm2, xterm).

---

**2. `\033c` & `\033[!p` — Hard and Soft Terminal Resets**

```c
// Hard Reset (RIS - Reset to Initial State)
printf("\033c");

// Soft Terminal Reset (DECSTR)
printf("\033[!p");
```

- **Hard Reset (`\033c` / RIS):** Equivalent to power-cycling or hard-rebooting a physical DEC terminal. It clears the screen, resets cursor position to `(0, 0)`, restores default tab stops, clears margins, and resets all colors/fonts.
- **Soft Reset (`\033[!p` / DECSTR):** Resets attributes (cursor visibility, character sets, scroll regions, origin mode) back to standard defaults without wiping the text currently visible on the screen.

---

**3. `\033(0` & `\033(B` — DEC Special Character / Box-Drawing Set**

Before Unicode and UTF-8 existed, terminals switched character maps using G0/G1 character set designations.

```c
// Switch to DEC Line Drawing Character Set
printf("\033(0");
printf("lqqqqk\n"); // Prints top corners and horizontal line: ┌────┐
printf("x    x\n"); // Prints vertical lines:                 │    │
printf("mqqqqj\n"); // Prints bottom corners:                  └────┘

// Switch back to standard US-ASCII
printf("\033(B");
```

- **History:** This is the underlying mechanism that `ncurses` historically used for `ACS_HLINE`, `ACS_VLINE`, `ACS_ULCORNER`, etc. Lowercase letters like `j`, `k`, `l`, `m`, `q`, and `x` map directly to corner, tee, and border line glyphs in the terminal's hardware ROM.

---

**4. `\033[?5h` / `\033[?5l` — Reverse Screen Video Mode (DECSCNM)**

```c
// Invert the entire screen (white background, black text)
printf("\033[?5h");

// Restore standard screen (black background, white text)
printf("\033[?5l");
```

- **What it does:** Flips the foreground and background rendition of the **entire screen buffer** at once, rather than just individual characters.
- **History:** Used in DEC terminals to toggle between "light background" and "dark background" modes without needing to rewrite any text.

---

**5. `\033[<top>;<bottom>r` — Hardware Scrolling Regions (DECSTBM)**

```c
// Set scrolling margin between lines 5 and 15
printf("\033[5;15r");

// Reset scrolling region to full screen
printf("\033[r");
```

- **What it does:** Defines a vertical "window" on the screen. When the cursor reaches the bottom of this region, only lines between `<top>` and `<bottom>` scroll up; rows 1–4 (headers) and rows 16+ (status bars) remain pinned in place.
- **History:** Allowed text editors and split-screen apps to scroll panes independently over slow serial lines (300 to 9600 baud) without repainting static headers or toolbars.

---

**6. `\033[?1049h` / `\033[?1049l` — Alternate Screen Buffer (xterm / DEC Private)**

```c
// Switch to alternate screen buffer and clear screen
printf("\033[?1049h");

// Return to primary screen buffer (restores previous terminal state)
printf("\033[?1049l");
```

- **What it does:** Modern terminal emulators maintain two independent screen buffers:
  1. **Primary Screen:** Your normal shell command history and scrollback.
  2. **Alternate Screen:** A clean workspace without a scrollback history.
- **Usage:** This is what full-screen terminal programs like `vim`, `less`, `htop`, and `ncurses` (`initscr()` / `endwin()`) invoke behind the scenes so your terminal returns to its exact state when the program exits.

---

**7. `\033[6n` — Device Status Report / Cursor Position Query (CPR)**

```c
// Ask terminal: "Where is the cursor?"
printf("\033[6n");
fflush(stdout);

// Terminal immediately writes back to stdin in format: \033[<row>;<col>R
```

- **What it does:** Triggers a two-way communication channel between terminal and program. The terminal reports its current row and column coordinates directly into the program's standard input stream (`stdin`).

---

**8. `\033]0;<title>\007` — Operating System Command (OSC) Window Title**

```c
// Change terminal window/tab title to "My C Application"
printf("\033]0;My C Application\007");
```

- **What it does:** Uses the OSC (`\033]`) introducer to set the window title or tab bar title of the terminal emulator. Terminated either by ASCII BEL (`\007` / `\a`) or the string terminator `ST` (`\033\\`).

---

**Quick Summary Reference Table**

| Sequence | Name | Category | Primary Use |
| :--- | :--- | :--- | :--- |
| `\033#8` | `DECALN` | DEC Line | Fill screen with `E`s (CRT alignment test) |
| `\033c` | `RIS` | Standard DEC | Hard reset of terminal state |
| `\033[!p` | `DECSTR` | Standard DEC | Soft reset of terminal attributes |
| `\033(0` / `\033(B` | `SCS` | VT100 Font | Switch between Line Drawing set and ASCII |
| `\033[?5h` / `l` | `DECSCNM` | DEC Private | Invert full screen video mode |
| `\033[<t>;<b>r`| `DECSTBM` | DEC VT100 | Confine scrolling to specific row range |
| `\033[?1049h`/`l`| `xterm` | DEC Private | Enter / leave alternate screen buffer |
| `\033[6n` | `CPR` | ANSI / DEC | Query current cursor `(x, y)` position |
| `\033]0;...\a` | `OSC 0` | xterm / Modern | Dynamically set window and tab title |
*/

#include <ncurses.h>

void alt_char_set() {
    printf("\n");
    printf("\033(0");
    printf("lqqqqk\n"); // Prints top corners and horizontal line: ┌────┐
    printf("x    x\n"); // Prints vertical lines:                  │    │
    printf("mqqqqj\n"); // Prints bottom corners:                  └────┘

    // Switch back to standard US-ASCII
    printf("\033(B");
}

int main(void) {

    // legacy DEC escapes. VT100
    printf("\033#8");  // fill the screen with the letter E
    printf("\033]0;Unicorn Window Title\a");  // set window/tab title
    printf("\033[?5hEnter any key to continue..."); // invert full screen video
    getchar(); // wait for key
    //
    // These work in my MacOS Terminal in xterm-256 color mode.
    // Double-width line: \033#6
    // Double-height top half: \033#3
    // Double-height bottom half: \033#4

    printf( "\033#6Double-width line\n");
    // 3. Double-height text (printed twice)
    printf( "\033#3BIG TITLE\n");
    printf( "\033#4BIG TITLE\n");

    alt_char_set();

    printf("\033[!pEnter any key to exit..."); // soft reset

    alt_char_set(); // switch to alt char set
    getchar(); // wait for key

    return 0;
}
