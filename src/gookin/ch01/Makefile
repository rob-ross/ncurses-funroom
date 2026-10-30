# Makefile for misc .c src Files

# A few variables
CC=clang
CFLAGS=-std=c23 -g
LIBS=-lncurses

SRC_DIR=.
EXE_DIR=../../../out/gookin/ch01

EXES = \
	${EXE_DIR}/01-02_goodbye-final


.PRECIOUS: %.o
.PHONY: all clean

all:    ${EXES}


# Ensure destination directory exists
${EXE_DIR}:
	mkdir -p ${EXE_DIR}

# for every item in EXES, this rule runs to make it. Depends on an object file with the same stem
# $@ means "copy everything to the left of the colon"
# $^ means "copy everything to the right of the colon"
${EXE_DIR}/%: %.o | ${EXE_DIR}
	${CC} -o $@ $< ${LIBS}


# make object files, compile if needed, .c is the prerequisite for the compile recipie but doesn't require making
%.o: ${SRC_DIR}/%.c
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	@rm -f ${EXES} 	@rm -f ${EXES}
