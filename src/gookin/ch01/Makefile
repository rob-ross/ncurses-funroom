# Makefile for misc .c src Files

# A few variables
CC=clang
CFLAGS=-std=c23 -g
LIBS=-lncurses

SRC_DIR=.
EXE_DIR=../../../out/gookin/ch01

EXES = \
	${EXE_DIR}/01-02_goodbye-final

OBJS = $(patsubst ${EXE_DIR}/%,${EXE_DIR}/%.o,${EXES})

.PRECIOUS: ${EXE_DIR}/%.o
.PHONY: all clean

all:    ${EXES}


# Ensure destination directory exists
${EXE_DIR}:
	mkdir -p ${EXE_DIR}

# for every item in EXES, this rule runs to make it. Depends on an object file with the same stem
${EXE_DIR}/%: ${EXE_DIR}/%.o
	${CC} -o $@ $< ${LIBS}


# make object files in EXE_DIR
${EXE_DIR}/%.o: ${SRC_DIR}/%.c | ${EXE_DIR}
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	@rm -f ${EXES} ${OBJS} *.o
