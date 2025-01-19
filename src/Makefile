CC=gcc
CFLAGS=-Wall -Wextra -ggdb -std=c99 -pedantic
BIN=compiler
DEPS=lexer.h parser.h util.h
SOURCES=lexer.o parser.o

all: compiler test

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
