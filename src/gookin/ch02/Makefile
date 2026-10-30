# Makefile for misc .c src Files

# A few variables
CC=clang
CFLAGS=-std=c23 -g
LIBS=-lncurses

SRC_DIR=.
EXE_DIR=../../../out/gookin/ch02

EXES = \
	${EXE_DIR}/02-02_add1 \
	${EXE_DIR}/02-03_add2 \
	${EXE_DIR}/02-04_add3 \
	${EXE_DIR}/02-05_yoda \
	${EXE_DIR}/02-06_typewriter \
	${EXE_DIR}/02-07_yourname \
	${EXE_DIR}/02-08_sushi \

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
