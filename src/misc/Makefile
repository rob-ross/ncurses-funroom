# Makefile for misc .c src Files

# A few variables
CC=clang
CFLAGS=-std=c23
LIBS=-lncurses

SRC_DIR=.
EXE_DIR=../../out/misc

EXES = \
	${EXE_DIR}/ansi_colors \
	${EXE_DIR}/char_displayer \
	${EXE_DIR}/manual_term_escapes \
	${EXE_DIR}/simple_color \


DATA_FILES = \
	${EXE_DIR}/Blocks.txt \
	${EXE_DIR}/EastAsianWidth.txt \

all:    ${EXES}

# Specific prerequisite: char_displayer requires the data files
${EXE_DIR}/char_displayer: ${DATA_FILES}

# Ensure destination directory exists
${EXE_DIR}:
	mkdir -p ${EXE_DIR}

# for every item in EXES, this rule runs to make it. Depends on an object file with the same stem
# $@ means "copy everything to the left of the colon"
# $^ means "copy everything to the right of the colon"
${EXE_DIR}/%: %.o | ${EXE_DIR}
	${CC} -o $@ $< ${LIBS}

# Rule to copy .txt data files to EXE_DIR
${EXE_DIR}/%.txt: ${SRC_DIR}/%.txt
	cp $< $@

# make object files, compile if needed, .c is the prerequisite for the compile recipie but doesn't require making
%.o: ${SRC_DIR}/%.c
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	@rm -f ${EXES} 	@rm -f ${EXES} ${DATA_FILES}
