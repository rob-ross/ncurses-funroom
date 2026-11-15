# Makefile for misc .c src Files

SANFLAGS = -fsanitize=address -fsanitize=leak


CC = clang
COMMON_DIR = ../common
CFLAGS = -std=c23 -g  ${SANFLAGS} -I${COMMON_DIR}
LIBS=-lncurses
LDFLAGS += $(SANFLAGS)

COMMON_SRCS = $(wildcard $(COMMON_DIR)/*.c)
COMMON_OBJS = $(patsubst $(COMMON_DIR)/%.c,$(EXE_DIR)/%.o,$(COMMON_SRCS))
SRC_DIR = .
EXE_DIR = ../../out/forms/

# Automatically discover all .c files in the source directory
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Derive the executable targets in EXE_DIR matching each source file stem
EXES = $(patsubst $(SRC_DIR)/%.c,$(EXE_DIR)/%,$(SRCS))

# Intermediate object files in EXE_DIR
OBJS = $(patsubst $(SRC_DIR)/%.c,$(EXE_DIR)/%.o,$(SRCS))

.PRECIOUS: ${EXE_DIR}/%.o
.PHONY: all clean

all: ${EXES}

# Ensure destination directory exists
${EXE_DIR}:
	mkdir -p ${EXE_DIR}

# Rule to link each executable from its matching object file
${EXE_DIR}/%: ${EXE_DIR}/%.o ${COMMON_OBJS}
	${CC} ${LDFLAGS} -o $@ $^ ${LIBS}

# Rule to compile each .c file into a .o file in EXE_DIR
${EXE_DIR}/%.o: ${SRC_DIR}/%.c | ${EXE_DIR}
	${CC} ${CFLAGS} -o $@ -c $<

${EXE_DIR}/%.o: ${COMMON_DIR}/%.c | ${EXE_DIR}
	${CC} ${CFLAGS} -o $@ -c $<

clean:
	@rm -f ${EXES} ${OBJS} *.o
