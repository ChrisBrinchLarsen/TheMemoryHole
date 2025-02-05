CC=gcc -g

# All the flags in the (CompSys) world
CFLAGS= -std=gnu11 -std=c11 -g3 -Wextra -Wall -pedantic

SRC = file.c test.sh
EXEC = file
VFLAGS = --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose

# Compile your program (make og make file)
prototype: prototype.c prototype.h
	$(CC) $(CFLAGS) prototype.h prototype.c -o prototype -lm

# Used for removing generated files (make clean)
clean:
	rm -f *.o
	rm -f file
	rm -f file.c~     # For emacs uses
	rm -rf file.dSYM  # For Mac users