# Makefile for c and cpp src Files


CC = clang
CFLAGS = -std=c23 -g

CXX = clang++
CXXFLAGS = -std=c++23 -g
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

# Intermediate object files in EXE_DIR
OBJS = $(patsubst %,$(EXE_DIR)/%.o,$(STEMS))

.PRECIOUS: ${EXE_DIR}/%.o
.PHONY: all clean

# Default rule

all:    ${EXES}

# Ensure destination directory exists
${EXE_DIR}:
	mkdir -p ${EXE_DIR}


# Rule to link object files into executable binary
${EXE_DIR}/%: ${EXE_DIR}/%.o
	${CXX} ${CXXFLAGS} -o $@ $< ${LIBS}

# Rule to compile .cpp source files into .o object files in EXE_DIR
${EXE_DIR}/%.o: ${SRC_DIR}/%.cpp | ${EXE_DIR}
	${CXX} ${CXXFLAGS} -c -o $@ $<

# Rule to compile each .c file into a .o file in EXE_DIR
${EXE_DIR}/%.o: ${SRC_DIR}/%.c | ${EXE_DIR}
	${CC} ${CFLAGS} -o $@ -c $<

# Clean up build artifacts
clean:
	@rm -f ${OBJS} ${EXES} *.o

