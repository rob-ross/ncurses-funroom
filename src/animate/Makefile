# src/animate/Makefile

# Makefile for C++ src Files

# A few variables


CC=clang++
CFLAGS=-std=c++23 -g
LIBS=-lncurses

SRC_DIR=.
EXE_DIR=../../out/animate

EXES = \
	${EXE_DIR}/simple_animate1 \
	${EXE_DIR}/simple_animate2 \
	${EXE_DIR}/simple_animate3 \
	${EXE_DIR}/simple_animate4 \

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
${EXE_DIR}/%.o: ${SRC_DIR}/%.cpp | ${EXE_DIR}
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	@rm -f ${EXES} ${OBJS} *.o
