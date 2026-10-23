**Upstream Contribution Workflow for GNU NCURSES Man Pages**

Correcting documentation directly at the upstream source produces the highest long-term return on effort. Because NCURSES powers terminal handling across virtually every Unix-like operating system, a single upstream documentation fix permanently eliminates ambiguity, prevents compounding developer errors, and propagates automatically to downstream distributions (macOS Homebrew, Debian, Fedora, Arch, FreeBSD) across the entire software ecosystem.

---

**1. Obtain the Master Development Source**

GNU NCURSES does not use pull requests on platforms like GitHub; Thomas Dickey maintains the master branch directly and publishes continuous weekly development snapshots.

To ensure your patch applies cleanly against the latest revision rather than outdated release tarballs:

* **Official Git Repository Mirror:**
  ```bash
  git clone https://github.com/ThomasDickey/ncurses-snapshots.git
  # or via Savannah:
  git clone https://git.savannah.gnu.org/git/ncurses.git
  ```
* **Weekly Snapshot Archives:**
  Source archives and rolling patches are available at `https://invisible-island.net/ncurses/`.

---

**2. Locate and Modify the Source Man Page**

The man pages are located inside the `man/` directory of the source tree. They are written in standard `groff`/`troff` using the `man` macro package.

1. **Find the target file:**
    - Section 3X routines: `man/curs_*.3x` (e.g., `man/curs_color.3x`, `man/curs_window.3x`)
    - General overview pages: `man/ncurses.3x`, `man/terminfo.5`
    - Utility pages: `man/tic.1m`, `man/infocmp.1m`

2. **Edit the source markup:**
    - Preserve existing macro conventions:
        - `.B` for bold / function names / macros (`.B init_extended_color`)
        - `.I` for italics / parameters (`.I color`, `.I pair`)
        - `.TP` for tagged lists / option descriptions
        - `.SH` for section headers
        - `.PP` for standard paragraph breaks

---

**3. Verify the Rendered Output Locally**

To ensure formatting precision and eliminate review friction before submission, test the rendered page directly with `man` or `nroff`:

```bash
# Render directly from the local file
man -l man/curs_color.3x

# Or render via groff/nroff
nroff -man man/curs_color.3x | less
```

Verify that line wraps, macro formatting, and parameter typography render without visual artifacts or broken cross-references.

---

**4. Generate a Clean Unified Diff**

Produce a clean patch relative to the root of the source tree:

```bash
git diff man/curs_color.3x > fix-curs_color-typo.patch
```

*(If working outside git, use `diff -u original_dir/man/curs_color.3x modified_dir/man/curs_color.3x > fix.patch`)*

---

**5. Submit to the Maintainer and Bug List**

To maximize the speed of integration and ensure transparent tracking, submit the patch via email to the official NCURSES channels:

* **Primary Mailing List:** `bug-ncurses@gnu.org`
* **Direct Maintainer Contact:** Thomas E. Dickey (`dickey@invisible-island.net`)

**Recommended Email Structure**

To minimize maintainer cognitive load and accelerate adoption:
1. **Subject:** `[PATCH] doc: fix <description> in <man_page_name>` (e.g., `[PATCH] doc: correct parameter range in curs_color(3x)`)
2. **Context:** A concise explanation of the factual inaccuracy, why the change improves clarity or correctness, and any supporting references (such as function signatures in header files or X/Open Curses standards).
3. **Attachment/Body:** Include the unified diff as an inline block or plain text attachment.

---

**Upstream vs. Downstream Impact**

While one could submit an issue to Homebrew (`homebrew-core`), downstream package managers will not maintain divergent man page patches indefinitely; they inevitably wait for upstream releases. Submitting fixes upstream resolves the root issue at the single point of truth, ensuring that every operating system and toolchain benefits simultaneously in subsequent release cycles.
