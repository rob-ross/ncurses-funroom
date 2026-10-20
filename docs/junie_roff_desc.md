**Documentation for Man Page Markup**

Man pages are written in **`roff`** (specifically processed today by **`groff`** or **`troff`**), using specialized **macro packages**. The specific dialect used in ncurses and traditional Linux/Unix man pages is the **`man` macro package** (also known as `groff_man` or `man(7)`). Another popular dialect, especially across BSD and macOS system utilities, is **`mdoc`** (`mdoc(7)`).

---

**Built-in Man Pages Documentation on Your Mac**

macOS includes reference manuals directly in terminal section 7 (Miscellaneous):

- **`man 7 man`** — Complete reference for the traditional `man` macro package (describes `.SH`, `.SS`, `.PP`, `.TP`, `.RS`, `.RE`, `\fB`, etc.).
- **`man 7 roff`** — Overview of the `roff` document formatting system, syntax rules, and low-level escape sequences.
- **`man 7 mdoc`** — Reference for the BSD semantic markup language used in modern BSD/macOS command manuals.

You can view them in your terminal with:
```bash
man 7 man
man 7 roff
man 7 mdoc
```

---

**Clarification of the Markup in Your Notes**

Here is a breakdown of the macros and escape sequences found in `docs/manpage_markup.txt`:

**1. Inline Font Escapes (`\f...`)**

These change font styles inline without adding line breaks:
- **`\fB`**: Switch font to **Bold**.
- **`\fI`**: Switch font to **Italic** (renders as underlined in terminal pagers).
- **`\fR`**: Switch font to **Roman** (regular / normal text).
- **`\fP`**: Switch font to **Previous** font (restores whatever font was active before the last `\f` switch).
    - *Example:* `\fBbold\fP` returns to the font before `\fB`.

**2. Line & Spacing Controls**

- **`.br`**: **Break** — forces a line break immediately without inserting vertical paragraph space.
- **`.sp [n]`**: **Space** — inserts vertical spacing (blank lines). `.sp` without arguments inserts 1 blank line; `.sp 2` inserts 2 lines.

**3. Paragraph & Section Macros**

- **`.SH "SECTION NAME"`**: **Section Heading** — starts a primary section (typically rendered bold and flush left).
- **`.SS "Subsection Name"`**: **Subsection Heading** — starts a subsection with a slight indent or lighter emphasis.
- **`.PP`** (or `.P` / `.LP`): **Paragraph Break** — resets indentation and begins a standard body paragraph with a preceding blank line.

**4. Standalone Font Macros**

- **`.B [text]`**: Sets text on the line in **Bold**. If arguments are omitted, the entire next line is bolded.
- **`.I [text]`**: Sets text in **Italic** (underlined).
- **`.BI`, `.BR`, `.RB`, `.IB`**: Alternating font macros (e.g., `.BR function (3)` bolds `function` and keeps `(3)` in Roman without spaces).

**5. Indentation & Lists**

- **`.RS [n]`**: **Relative Shift / Start** — moves the left margin to the right by `n` characters (default 0.5 inches / ~4-5 characters). Useful for nested code blocks or sub-lists.
- **`.RE`**: **Relative End** — moves the left margin back to the previous indent level.
- **`.TP [n]`**: **Tagged Paragraph (Hanging Indent)** — creates a definition list. The next line is the *tag/label*, and the following lines are the *definition/body* indented by `n` spaces.
  ```roff
  .TP 5
  \fBinitscr\fR
  Initializes the curses system and allocates memory.
  ```
- **`.bP`**: **Bullet Point** — outputs a bullet symbol (`•` or `*`) and sets up a list item.

---

**Standard Man Page Structure**

Standard man pages follow a conventional section layout:

- **`.SH NAME`**: Name and one-line summary (`program \- short description`), used by `whatis` and `apropos`.
- **`.SH SYNOPSIS`**: Function prototypes or CLI command-line syntax.
- **`.SH DESCRIPTION`**: Detailed explanation of functions, parameters, and behaviors.
- **`.SH RETURN VALUE`**: Return values, exit codes, and error semantics (e.g., `OK` or `ERR`).
- **`.SH NOTES`**: Caveats, macro vs. function notes, reentrancy, or signal safety.
- **`.SH PORTABILITY`**: Compliance with X/Open Curses, SVr4 curses, POSIX standards.
- **`.SH SEE ALSO`**: References to related manual pages (e.g., `ncurses(3X)`, `curs_window(3X)`).

---

**Recommended Online Documentation**

- **[Man-pages project (man7.org)](https://man7.org/linux/man-pages/man7/groff_man.7.html)**: The definitive web documentation for Linux/Unix `groff_man(7)`.
- **[GNU Troff (Groff) Manual](https://www.gnu.org/software/groff/manual/)**: Detailed specification of macro packages and low-level roff formatting.
