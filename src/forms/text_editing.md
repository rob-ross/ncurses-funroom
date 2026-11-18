

**1. Why `Cmd+X` and Native Text Selection Failed**

In `native_text_handling.c`, disabling mouse tracking allows the macOS terminal emulator to manage mouse highlighting locally. However, this creates a strict boundary between two independent systems:

```
+--------------------------------------------------------------+
|                     macOS Terminal Window                    |
|  - Renders character cells on screen                        |
|  - Tracks mouse clicks and paints native highlight           |
+--------------------------------------------------------------+
                               |
              One-Way Stream over stdin / pty
                               |
                               v
+--------------------------------------------------------------+
|                 Your C Application State                     |
|  - Maintains `char buffer[]` and `int cursor_pos`            |
|  - Receives only raw byte keycodes (e.g., ASCII 127)         |
+--------------------------------------------------------------+
```

1. **Why `Cmd+X` (Cut) Does Nothing**: In terminal emulators, the screen is an output grid rather than an editable document canvas. The terminal emulator never permits GUI cut operations on its terminal grid; therefore, `Cmd+X` is either ignored by the terminal window manager or emits a beep.
2. **Why Pressing `Delete` Deleted at `cursor_pos` Instead of the Selection**: The terminal emulator's native highlight exists purely in the emulator's local video buffer. When you press `Delete`, the emulator sends an ASCII `127` byte to `stdin`. Your C program receives `127`, checks `cursor_pos`, and deletes `buffer[cursor_pos - 1]`. Because your C program has zero knowledge of the emulator's visual highlight, it cannot delete the selected range.

---

**2. Why Up/Down and Shift-Arrow Produced Visual Artifacts (`^C`, `~I`, `~R`)**

The visual anomalies and line corruption you observed occur due to **unhandled multi-byte escape sequences and integer truncation**:

**A. Up/Down Arrows Producing `^C`**

- In `ncurses`, `KEY_UP` is defined as integer constant `259` (`0403` octal), and `KEY_DOWN` is `258`.
- In `native_text_handling.c`, there are no `case KEY_UP:` or `case KEY_DOWN:` branches.
- When `wgetch()` returns `259`, it falls through unhandled. When converted or cast to a single 8-bit `char` (`(char)259`), it wraps around (`259 % 256 = 3`).
- In ASCII, `3` is the `ETX` (End of Text / `Ctrl+C`) character. When printed to an ncurses window, ncurses displays control character `3` as **`^C`**, corrupting your text buffer with non-printable control bytes.

**B. Shift-Arrow Keys Producing `~I` and `~R`**

- When you hold `Shift` with arrow keys, terminal emulators emit extended VT220/xterm escape sequences such as `\033[1;2D` or `\033[a`.
- If the active terminfo database lacks entries for shifted arrows (e.g., `kLFT2`), `ncurses` cannot map the entire escape sequence to a single integer token.
- `wgetch()` consumes the leading `\033[` as an escape chord, but the trailing parameters (`1`, `;`, `2`, `D`, `~`, `I`, `R`) are dispatched as individual printable keystrokes, which your `isprint(ch)` branch appends into the text buffer.

---

**3. The Optimal Model for Terminal Text Entry: GNU Readline / Emacs Standard**

The terminal ecosystem standardized on **GNU Readline** (the input engine powering `bash`, `zsh`, `gdb`, `python`, `sqlite3`, and `lldb`). Emulating GNU Readline yields the highest return on investment: every Unix user already has these shortcuts encoded in muscle memory.

**Core Readline Shortcuts for Single-Line Prompts**

| Goal | Shortcut | Key Code / Sequence | Behavior |
| :--- | :--- | :--- | :--- |
| **Beginning of Line** | `Ctrl+A` or `Home` | `1` / `KEY_HOME` | Move cursor to index `0` |
| **End of Line** | `Ctrl+E` or `End` | `5` / `KEY_END` / `KEY_LL` | Move cursor to `buf_len` |
| **Move Backward 1 Char** | `Ctrl+B` or `Left Arrow` | `2` / `KEY_LEFT` | `cursor_pos--` |
| **Move Forward 1 Char** | `Ctrl+F` or `Right Arrow` | `6` / `KEY_RIGHT` | `cursor_pos++` |
| **Move Backward 1 Word** | `Alt+B` / `Option+Left` | `\033` followed by `'b'` | Jump to previous whitespace boundary |
| **Move Forward 1 Word** | `Alt+F` / `Option+Right` | `\033` followed by `'f'` | Jump to next whitespace boundary |
| **Delete Prev Character** | `Backspace` / `Ctrl+H` | `127` / `\b` / `8` / `KEY_BACKSPACE` | Delete character before cursor |
| **Delete Next Character** | `Delete` / `Ctrl+D` | `4` / `KEY_DC` | Delete character under cursor |
| **Cut to End of Line** | `Ctrl+K` ("Kill") | `11` | Erase from cursor to `buf_len`, store in kill-ring |
| **Cut to Start of Line** | `Ctrl+U` | `21` | Erase from index `0` to cursor, store in kill-ring |
| **Cut Previous Word** | `Ctrl+W` | `23` | Erase preceding word, store in kill-ring |
| **Paste / Yank** | `Ctrl+Y` | `25` | Insert last killed text at cursor position |
| **History Navigation** | `Up` / `Down` Arrow | `KEY_UP` / `KEY_DOWN` | Cycle through previous input strings |

---

**4. Terminal Text Editors to Study**

To understand how modern terminal applications maximize text editing utility, examine these two benchmarks:

1. **GNU Nano** (Shipped with macOS / Linux):
    - **Paradigm**: Modeless, straightforward editing.
    - **Strengths**: Explicit hotkey bar displayed at the bottom of the screen (`Ctrl+O` WriteOut, `Ctrl+R` Read File, `Ctrl+K` Cut Text, `Ctrl+U` Paste Text, `Ctrl+6` Set Selection Mark).
    - **Documentation**: Running `nano` and pressing `Ctrl+G` opens its built-in keyboard manual.

2. **Micro** (`micro-editor.github.io`):
    - **Paradigm**: Modern terminal editor with full desktop-style keybindings.
    - **Strengths**: Supports standard `Ctrl+C` (Copy), `Ctrl+V` (Paste), `Ctrl+X` (Cut), `Ctrl+Z` (Undo), full mouse selection, and multi-cursor editing out of the box.
    - **Keybinding Reference**: Built-in command `> help defaultkeys` lists every action and binding.

---

**Recommended Strategy for Your Command Bar**

For a single-line input field in an ncurses application:
1. **Filter Unhandled Special Keys**: Explicitly catch `KEY_UP`, `KEY_DOWN`, `KEY_PPAGE`, `KEY_NPAGE`, and escape sequences so they never fall through to `isprint()`.
2. **Implement Readline Essentials**: Support `Ctrl+A`, `Ctrl+E`, `Ctrl+W`, `Ctrl+K`, `Ctrl+U`, and arrow navigation.
3. **Use Up/Down for History Recall**: Map `KEY_UP` and `KEY_DOWN` to cycle through an array of previous command strings rather than moving vertically.
4. **Use Bracketed Paste for External Text**: Capture `\033[200~` payloads so users can paste clipboard content with `Cmd+V` reliably.
