CC=gcc
CFLAGS=-Wall -Wextra -ggdb -std=c11 -pedantic
BIN=compiler
DEPS=util.h lexer.h parser.h codegen.h hashtable.h analysis.h asm.h
SOURCES=lexer.o parser.o codegen.o hashtable.o analysis.o asm.o

all: compiler test memtest

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
