# Makefile for misc .c src Files

CC = clang
CFLAGS = -std=c23 -g
LIBS = -lncurses

SRC_DIR = .
EXE_DIR = ../../../out/gookin/ch07

# Automatically discover all .c files in the source directory
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Derive the executable targets in EXE_DIR matching each source file stem
EXES = $(patsubst $(SRC_DIR)/%.c,$(EXE_DIR)/%,$(SRCS))

# Intermediate object files
OBJS = $(patsubst $(SRC_DIR)/%.c,%.o,$(SRCS))

.PRECIOUS: %.o
.PHONY: all clean

all: ${EXES}

# Ensure destination directory exists
${EXE_DIR}:
	mkdir -p ${EXE_DIR}

# Rule to link each executable from its matching object file
${EXE_DIR}/%: %.o | ${EXE_DIR}
	${CC} -o $@ $< ${LIBS}

# Rule to compile each .c file into a .o file
%.o: ${SRC_DIR}/%.c
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	@rm -f ${EXES} ${OBJS}
