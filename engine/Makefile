CC=gcc -g

# All the flags in the (CompSys) world
CFLAGS= -std=gnu11 -g3 -Wextra -Wall #-pedantic -std=gnu11

# Compile your program (make og make file)
unit: assembly.o cache.o setup.o memory.o mmu.o read_exec.o simulate.o unit.o hashmap/hashmap.o
	$(CC) $(CFLAGS) assembly.o cache.o setup.o memory.o mmu.o read_exec.o simulate.o unit.o hashmap/hashmap.o -o unit -lm

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

sim: assembly.o cache.o main.o setup.o memory.o mmu.o read_exec.o simulate.o hashmap/hashmap.o
	$(CC) $(CFLAGS) assembly.o cache.o main.o setup.o memory.o mmu.o read_exec.o simulate.o hashmap/hashmap.o -o sim -lm

%.riscv: %.c lib.c Makefile
	/opt/riscv/bin/riscv32-unknown-elf-gcc  -march=rv32im -mabi=ilp32 -fno-tree-loop-distribute-patterns -mno-relax -Og $< lib.c -static -nostartfiles -nostdlib -o $@ -g

%.dis: %.riscv Makefile
	/opt/riscv/bin/riscv32-unknown-elf-objdump -s -w $< > $@
	/opt/riscv/bin/riscv32-unknown-elf-objdump -S $< >> $@

all: sim
rebuild: clean all

# Used for removing generated files (make clean)
clean:
	rm -f prototype
	rm -f unit
	rm -f *.o
	rm -f file
	rm -f accesses
	rm -f sim
	rm -f loggers
	rm -f cache_log
	rm -f output