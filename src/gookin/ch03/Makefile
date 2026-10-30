# Makefile for misc .c src Files

CC = clang
CFLAGS = -std=c23 -g
LIBS = -lncurses

SRC_DIR = .
EXE_DIR = ../../../out/gookin/ch03

# Automatically discover all .c files in the source directory
SRCS = $(wildcard $(SRC_DIR)/*.c)

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
