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
expand.h 	  		\

SOURCES=	  		\
lexer.o       		\
parser.o      		\
codegen.o     		\
diagnostics.o 		\
hashtable.o   		\
symboltable.o 		\
types.o 			\
expand.o 	  		\

PROTO=./test/main.sn

all: compiler

compiler: main.o $(SOURCES)
	@$(CC) $(CFLAGS) $^ -o $(BIN)
	@echo LINK $^

run: compiler
	@echo "Running Prototype $(PROTO)"
	@./$(BIN) -t asm $(PROTO) --dump-ast
	@./$(BIN) -t run $(PROTO)

debug: compiler
	@echo "Debugging Prototype $(PROTO)"
	@gdb 								\
		-ex "file ./$(BIN)" 			\
		-ex "break main"    			\
		-ex "run $(PROTO)"

# Random assembly program for prototyping/testing
dev-asm: dev.asm
	nasm -felf64 $< -gdwarf
	cc dev.o -no-pie -ggdb -lc -lraylib -lglfw
	./a.out

test: ./test/test.sn compiler
	@echo "TEST $<"
	@./$(BIN) $< -t obj
	@$(CC) $(CFLAGS) -o test/test test/test.c test/test.o
	@./test/test

%.o: %.c Makefile $(DEPS)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm *.o $(BIN)
	rm test/{*.o,*.s,test}

.PHONY: clean, test
