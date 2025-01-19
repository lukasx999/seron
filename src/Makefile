CC=gcc
CFLAGS=-Wall -Wextra -ggdb -std=c23 -pedantic
BIN=compiler
DEPS=lexer.h util.h
SOURCES=lexer.o

all: compiler

compiler: main.o $(SOURCES)
	@$(CC) $(CFLAGS) $^ -o $(BIN)
	@echo CC $^

test: test.o $(SOURCES)
	@$(CC) $(CFLAGS) $^ -o test
	@echo CC $^
	@./test
	@rm test

%.o: %.c Makefile $(DEPS)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm *.o $(BIN)

.PHONY: clean
