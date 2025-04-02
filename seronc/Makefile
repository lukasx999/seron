CC=gcc
CFLAGS=-Wall -Wextra -ggdb -std=c11 -pedantic -fsanitize=address,undefined

BIN=seronc

DEPS=\
main.h        \
util.h        \
lexer.h       \
parser.h      \
gen.h 	      \
backend.h     \
symboltable.h \
types.h       \
ast.h         \

SOURCES=\
lexer.o       \
parser.o      \
gen.o         \
backend.o     \
util.o        \
types.o       \
symboltable.o \
ast.o         \

all: compiler

compiler: main.o $(SOURCES)
	@$(CC) $(CFLAGS) $^ -o $(BIN)
	@echo LINK $^

test: compiler
	@echo TEST test/main.srn
	@./seronc ./test/test.srn -Ov --debug-asm

	@$(CC) $(CFLAGS) -o test/test test/test.c test/test.o
	@./test/test

%.o: %.c Makefile $(DEPS)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm *.o $(BIN)
	rm test/{*.o,*.s,test}

.PHONY: clean test
