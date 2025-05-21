CC=gcc
CFLAGS=-Wall -Wextra -ggdb -std=c11 -pedantic -fsanitize=address,undefined -Ilib

BIN=seronc

DEPS=		   		\
main.h         		\
diagnostics.h  		\
lexer.h        		\
parser.h       		\
codegen.h      		\
hashtable.h    		\
symboltable.h  		\
types.h 	  		\
colors.h 	  		\

SOURCES=	  		\
lexer.o       		\
parser.o      		\
codegen.o     		\
diagnostics.o 		\
hashtable.o   		\
symboltable.o 		\

all: compiler

compiler: main.o $(SOURCES)
	@$(CC) $(CFLAGS) $^ -o $(BIN)
	@echo LINK $^

debug: compiler
	gdb 								\
		-ex "file ./seronc" 			\
		-ex "break main"    			\
		-ex "run ./test/main.srn"

# Random assembly program for prototyping/testing
dev-asm: dev.asm
	nasm -felf64 $< -gdwarf
	ld dev.o

test: compiler
	./$(BIN) ./test/test.sn -cv
	$(CC) $(CFLAGS) -o test/test test/test.c test/test.o
	./test/test

%.o: %.c Makefile $(DEPS)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm *.o $(BIN)
	rm test/{*.o,*.s,test}

.PHONY: clean, test
