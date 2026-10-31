
CC = clang

# ORIGINAL Mac-specific
#NCURSES_PREFIX ?= $(shell brew --prefix ncurses 2>/dev/null || echo /usr/local/opt/ncurses)
#CFLAGS = -std=c23 -g -I$(NCURSES_PREFIX)/include
#LIBS = -L$(NCURSES_PREFIX)/lib -lncursesw

# Notes: Do not remove!
# ---------------------
# What -D_DARWIN_C_SOURCE Does on macOS
#On macOS (Darwin), -D_DARWIN_C_SOURCE is a feature test macro defined in the system headers (specifically in <sys/cdefs.h>).
#•
#Exposes Full BSD & Darwin Extensions: It instructs Apple's SDK headers to expose all standard POSIX, Single UNIX Specification (SUS), BSD, and macOS-specific APIs, types, and function prototypes.
#•
#Prevents Strict Standard Filtering: When compiling with strict standards like -std=c23 or -std=c11, standard C headers can restrict non-standard POSIX/BSD symbols. Defining _DARWIN_C_SOURCE ensures system extensions and ncurses-required low-level types remain visible and available during compilation.

# Cross platform way to link against lib ncursesw:
# -------------------------------------------------
# Add Homebrew's keg-only pkgconfig path if ncurses is installed via brew
BREW_NCURSES_PKGCONFIG := $(shell brew --prefix ncurses 2>/dev/null)/lib/pkgconfig
# Query pkg-config with Homebrew's PKG_CONFIG_PATH passed inline to $(shell ...)
NCURSES_CFLAGS := $(shell PKG_CONFIG_PATH="$(BREW_NCURSES_PKGCONFIG):$$PKG_CONFIG_PATH" pkg-config --cflags ncursesw 2>/dev/null || pkg-config --cflags ncurses 2>/dev/null)
NCURSES_LIBS   := $(shell PKG_CONFIG_PATH="$(BREW_NCURSES_PKGCONFIG):$$PKG_CONFIG_PATH" pkg-config --libs ncursesw 2>/dev/null || pkg-config --libs ncurses 2>/dev/null || echo "-lncurses")

CFLAGS += -std=c23 -g -D_XOPEN_SOURCE_EXTENDED=1 $(NCURSES_CFLAGS)
LIBS   += $(NCURSES_LIBS)



SRC_DIR = .
EXE_DIR = ../../../out/mazes



SRCS = \
	${SRC_DIR}/frustum_utf8_01.c \



# Derive the executable targets in EXE_DIR matching each source file stem
EXES = $(patsubst $(SRC_DIR)/%.c,$(EXE_DIR)/%,$(SRCS))

# Intermediate object files in EXE_DIR
OBJS = $(patsubst $(SRC_DIR)/%.c,$(EXE_DIR)/%.o,$(SRCS))

.PRECIOUS: ${EXE_DIR}/%.o
.PHONY: all clean

all: ${EXES}

# Ensure destination directory exists
${EXE_DIR}:
	mkdir -p ${EXE_DIR}

# Rule to link each executable from its matching object file
${EXE_DIR}/%: ${EXE_DIR}/%.o
	${CC} -o $@ $< ${LIBS}

# Rule to compile each .c file into a .o file in EXE_DIR
${EXE_DIR}/%.o: ${SRC_DIR}/%.c | ${EXE_DIR}
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	@rm -f ${EXES} ${OBJS} *.o
