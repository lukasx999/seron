CC=gcc
CFLAGS=-Wall -Wextra -ggdb -std=c11 -pedantic -fsanitize=address,undefined

BIN=seronc

DEPS=\
main.h      \
util.h        \
lexer.h       \
parser.h      \
gen.h 	      \
backend.h     \
symboltable.h \
grammar.h	  \
types.h       \
ast.h         \
layout.h      \

SOURCES=\
lexer.o       \
parser.o      \
gen.o         \
backend.o     \
util.o        \
types.o       \
symboltable.o \
grammar.o     \
ast.o         \
layout.o      \


all: compiler

compiler: main.o $(SOURCES)
	@$(CC) $(CFLAGS) $^ -o $(BIN)
	@echo LINK $^

%.o: %.c Makefile $(DEPS)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm *.o $(BIN)

.PHONY: clean
