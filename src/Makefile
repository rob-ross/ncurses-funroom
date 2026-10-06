# Makefile for src Files

# A few variables


CC=clang
CFLAGS=-std=c23
LIBS=-lncurses

SRC_DIR=.
EXE_DIR=../demo/exe

EXES = \
	${EXE_DIR}/simple_color \
	${EXE_DIR}/manual_term_escapes \
	${EXE_DIR}/char_displayer \


# for every item in EXES, this rule runs to make it. Depends on an object file with the same stem
# $@ means "copy everything to the left of the colon"
# $^ means "copy everything to the right of the colon"
${EXE_DIR}/%: %.o
	${CC} -o $@ $< ${LIBS}

# make object files, compile if needed, .c is the prerequisite for the compile recipie but doesn't require making
%.o: ${SRC_DIR}/%.c
	${CC} ${CFLAGS} -o $@ -c $<

all:    ${EXES}

clean:
	@rm -f ${EXES}
