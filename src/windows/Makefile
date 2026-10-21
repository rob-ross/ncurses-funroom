# Makefile for src Files

# A few variables


CC=clang++
CFLAGS=-std=c++23
LIBS=-lncurses

SRC_DIR=.
EXE_DIR=../../out/windows

EXES = \
	${EXE_DIR}/window1 \
	${EXE_DIR}/window2 \
	${EXE_DIR}/resize_handling \
	${EXE_DIR}/resize_newwin \

# the next line doesn't work because it gets parsed as "window1.cpp, window2.cpp,..." instead of
# "window1, window2, ..."
#	${EXE_DIR}/$(wildcard window*) \

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
%.o: ${SRC_DIR}/%.cpp
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	@rm -f ${EXES} 	@rm -f ${EXES}
