# Makefile for misc .c src Files

CC = clang
NCURSES_PREFIX ?= $(shell brew --prefix ncurses 2>/dev/null || echo /usr/local/opt/ncurses)
CFLAGS = -std=c23 -g -I$(NCURSES_PREFIX)/include
LIBS = -L$(NCURSES_PREFIX)/lib -lncursesw


SRC_DIR = .
EXE_DIR = ../../out/mazes

# Automatically discover all .c files in the source directory
#SRCS = $(wildcard $(SRC_DIR)/*.c)

SRCS = \
	${SRC_DIR}/maze1.c \
	${SRC_DIR}/maze2.c \
	${SRC_DIR}/maze3.c \
	${SRC_DIR}/maze4.c \
	${SRC_DIR}/maze5.c \
	${SRC_DIR}/maze6.c \
	${SRC_DIR}/maze7.c \
	${SRC_DIR}/maze8.c \
	${SRC_DIR}/maze9.c \



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
