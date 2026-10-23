# Makefile for misc .c src Files

# A few variables
CC=clang
CFLAGS=-std=c23
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
