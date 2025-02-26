CC=gcc -g

# All the flags in the (CompSys) world
CFLAGS= -std=c23 -g3 -Wextra -Wall -lm #-pedantic -std=gnu11

# Compile your program (make og make file)
prototype: prototype.c prototype.h
	$(CC) $(CFLAGS) prototype.h prototype.c -o prototype.exe

unit: cache.o memory.o mmu.o unit.o 
	$(CC) $(CFLAGS) cache.o memory.o mmu.o unit.o -o unit.exe 

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

sim: assembly.o cache.o main.o memory.o mmu.o read_exec.o simulate.o
	$(CC) $(CFLAGS) assembly.o cache.o main.o memory.o mmu.o read_exec.o simulate.o -o sim

%.riscv: %.c lib.c Makefile
	/opt/riscv/bin/riscv32-unknown-elf-gcc  -march=rv32im -mabi=ilp32 -fno-tree-loop-distribute-patterns -mno-relax -O1 $< lib.c -static -nostartfiles -nostdlib -o $@

%.dis: %.riscv Makefile
	/opt/riscv/bin/riscv32-unknown-elf-objdump -s -w $< > $@
	/opt/riscv/bin/riscv32-unknown-elf-objdump -S $< >> $@

all: sim
rebuild: clean all

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
	rm -f accesses
	rm -f sim
	rm -f loggers
	rm -f cache_log
	rm -f output