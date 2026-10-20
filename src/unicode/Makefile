# Makefile for src Files

# A few variables


CXX = clang++
CXXFLAGS = -std=c++23
# Linux needs to link against -lncursesw
LIBS =-lncurses

SRC_DIR=.
EXE_DIR=../../out/unicode

EXES = \
	${EXE_DIR}/udisplay_1 \

# Default rule

all:    ${EXES}

# Ensure destination directory exists
${EXE_DIR}:
	mkdir -p ${EXE_DIR}


# Rule to link object files into executable binary
${EXE_DIR}/%: %.o | ${EXE_DIR}
	${CXX} ${CXXFLAGS} -o $@ $< ${LIBS}

# Rule to compile .cpp source files into .o object files
%.o: ${SRC_DIR}/%.cpp
	${CXX} ${CXXFLAGS} -c -o $@ $<

# Clean up build artifacts
clean:
	@rm -f *.o ${EXES}

.PHONY: all clean
