CC=gcc -g

# All the flags in the (CompSys) world
CFLAGS= -std=gnu11 -std=c11 -g3 -Wextra -Wall -lm #-pedantic

# Compile your program (make og make file)
prototype: prototype.c prototype.h
	$(CC) $(CFLAGS) prototype.h prototype.c -o prototype.exe

unit: cache.o memory.o unit.o 
	$(CC) $(CFLAGS) cache.o memory.o unit.o -o unit.exe 

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c memory.c -o memory.o

cache.o: cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c -o cache.o

unit.o: unit.c
	$(CC) $(CFLAGS) -c unit.c -o unit.o

%.riscv: %.c lib.c Makefile
	/opt/riscv/bin/riscv32-unknown-elf-gcc  -march=rv32im -mabi=ilp32 -fno-tree-loop-distribute-patterns -mno-relax -O1 $< lib.c -static -nostartfiles -nostdlib -o $@

%.dis: %.riscv Makefile
	/opt/riscv/bin/riscv32-unknown-elf-objdump -s -w $< > $@
	/opt/riscv/bin/riscv32-unknown-elf-objdump -S $< >> $@

all: sim
rebuild: clean all

# sim uses simulate
sim: *.c 
	$(CC) $(CFLAGS) *.c -o sim 

%.riscv: %.c lib.c Makefile
	/opt/riscv/bin/riscv32-unknown-elf-gcc-12.1.0 -march=rv32im -mabi=ilp32 -fno-tree-loop-distribute-patterns -mno-relax -O2 $< lib.c -static -nostartfiles -nostdlib -o $@

%.riscv: %.s Makefile
	/opt/riscv/bin/riscv32-unknown-elf-gcc-12.1.0 -march=rv32im -mabi=ilp32 -fno-tree-loop-distribute-patterns -mno-relax -O2 $< -static -nostartfiles -nostdlib -o $@

%.dis: %.riscv Makefile
	/opt/riscv/bin/riscv32-unknown-elf-objdump -s -w $< > $@

# Used for removing generated files (make clean)
clean:
	rm -f prototype.exe
	rm -f *.o
	rm -f file
	rm -f file.c~     # For emacs uses
	rm -rf file.dSYM  # For Mac users
	rm -rf *.o sim  vgcore*
	/opt/riscv/bin/riscv32-unknown-elf-objdump -S $< >> $@