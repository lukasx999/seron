CC=gcc
CFLAGS=-Wall -Wextra -ggdb -std=c11 -pedantic -Wdouble-promotion -fsanitize=undefined -fsanitize-trap

BIN=compiler

DEPS=\
util.h        \
lexer.h       \
parser.h      \
gen.h 	      \
backend.h     \
symboltable.h \
grammar.h     \
ast.h         \

SOURCES=\
lexer.o       \
parser.o      \
gen.o         \
backend.o     \
util.o        \
symboltable.o \
grammar.o     \
ast.o         \



all: compiler memtest

compiler: main.o $(SOURCES)
	@$(CC) $(CFLAGS) $^ -o $(BIN)
	@echo CC $^

test: tests/test.o $(SOURCES)
	@$(CC) $(CFLAGS) $^ -o test
	@echo CC $^
	@./test
	@rm test
	@rm $<

memtest:
	@./tests/memtest.sh ./$(BIN)

%.o: %.c Makefile $(DEPS)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm *.o $(BIN)

.PHONY: clean, memtest
