# Makefile for src Files

# A few variables


CC=clang++
CFLAGS=-std=c++23 -g
LIBS=-lncurses

SRC_DIR=.
EXE_DIR=../../out/windows

EXES = \
	${EXE_DIR}/window1 \
	${EXE_DIR}/window2 \
	${EXE_DIR}/resize_handling \
	${EXE_DIR}/resize_newwin \

OBJS = $(patsubst ${EXE_DIR}/%,${EXE_DIR}/%.o,${EXES})

.PRECIOUS: ${EXE_DIR}/%.o
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

# make object files in EXE_DIR
${EXE_DIR}/%.o: ${SRC_DIR}/%.cpp | ${EXE_DIR}
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	@rm -f ${EXES} ${OBJS} *.o
