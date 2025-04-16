CC=gcc
CFLAGS=-Wall -Wextra -ggdb -std=c11 -pedantic -fsanitize=address,undefined

BIN=seronc

DEPS=		  \
main.h        \
util.h        \
lexer.h       \
parser.h      \
codegen.h     \
hashtable.h   \
symboltable.h \

SOURCES=	  \
lexer.o       \
parser.o      \
codegen.o     \
util.o        \
hashtable.o   \
symboltable.o \

all: compiler

compiler: main.o $(SOURCES)
	@$(CC) $(CFLAGS) $^ -o $(BIN)
	@echo LINK $^

# Remove me!
some: some.asm
	nasm -felf64 $< -gdwarf && ld some.o

test: compiler
	@echo TEST test/main.srn
	@./seronc ./test/test.srn -cv

	@$(CC) $(CFLAGS) -o test/test test/test.c test/test.o
	@./test/test

%.o: %.c Makefile $(DEPS)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm *.o $(BIN)
	rm test/{*.o,*.s,test}

.PHONY: clean test
