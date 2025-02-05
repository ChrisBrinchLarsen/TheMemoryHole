CC=gcc -g

# All the flags in the (CompSys) world
CFLAGS= -pedantic -std=gnu11 -std=c11 -pedantic -g3 # -Werror -Wextra -Wall

SRC = file.c test.sh
EXEC = file
VFLAGS = --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose

# Compile your program (make og make file)
prototype: prototype.c Makefile
	$(CC) $(CFLAGS) -o prototype prototype.c

# Used for removing generated files (make clean)
clean:
	rm -f *.o
	rm -f file
	rm -f file.c~     # For emacs uses
	rm -rf file.dSYM  # For Mac users