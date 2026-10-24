# Makefile for c and cpp src Files


CC = clang
CFLAGS = -std=c23

CXX = clang++
CXXFLAGS = -std=c++23
# Linux needs to link against -lncursesw
LIBS =-lncurses

SRC_DIR=.
EXE_DIR=../../out/unicode

# Automatically discover all .c files in the source directory
SRCS = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*.cpp )

# Extract file stems (e.g., ./foo.c -> foo )
STEMS = $(notdir $(basename $(SRCS)))

# Derive the executable targets in EXE_DIR matching each source file stem
EXES = $(patsubst %,$(EXE_DIR)/%,$(STEMS))

# Intermediate object files
OBJS = $(patsubst %,%.o,$(STEMS))

.PHONY: all clean

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

# Rule to compile each .c file into a .o file
%.o: ${SRC_DIR}/%.c
	${CC} ${CFLAGS} -o $@ -c $<

# Clean up build artifacts
clean:
	@rm -f *.o ${EXES}

