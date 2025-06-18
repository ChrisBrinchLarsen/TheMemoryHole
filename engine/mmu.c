#include "mmu.h"
#include "memory.h"
#include <stdio.h>
#include <stdint.h>

struct memory* mem;
FILE* accesses;

void open_accesses_file() {
    accesses = fopen("accesses", "a");
}
void close_accesses_file() {
    fclose(accesses);
}

uint32_t mmu_get_checksum() {
    return get_cache_checksum();
}

void mmu_wr_w_instr(struct memory *mem, int addr, uint32_t data) {
    
    if (accesses)
        fprintf(accesses, "mmu_wr_w_instr(memory, 0x%x, %d);\n", addr, data);

    if (addr & 0b11)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    memory_wr_w(mem, addr, data);
}

void mmu_wr_h_instr(struct memory *mem, int addr, uint16_t data) {
    
    if (accesses)
        fprintf(accesses, "mmu_wr_h_instr(memory, 0x%x, %d);\n", addr, data);
    
    if (addr & 0b1)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    memory_wr_h(mem, addr, data);
}

void mmu_wr_b_instr(struct memory *mem, int addr, uint8_t data) {
    if (accesses)
        fprintf(accesses, "mmu_wr_b_instr(memory, 0x%x, %d);\n", addr, data);
    
     memory_wr_b(mem, addr, data);
}

void mmu_wr_w(struct memory *mem, int addr, uint32_t data) {
    
    if (accesses)
        fprintf(accesses, "mmu_wr_w(memory, 0x%x, %d);\n", addr, data);

    if (addr & 0b11)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    cache_wr_w(mem, addr, data);
}

void mmu_wr_h(struct memory *mem, int addr, uint16_t data) {
    if (accesses)
        fprintf(accesses, "mmu_wr_h(memory, 0x%x, %d);\n", addr, data);

    if (addr & 0b1)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    cache_wr_h(mem, addr, data);
}

void mmu_wr_b(struct memory *mem, int addr, uint8_t data) {
    fprintf(accesses, "mmu_wr_b(memory, 0x%x, %d);\n", addr, data);
    cache_wr_b(mem, addr, data);
}

int mmu_rd_instr(struct memory *mem, int addr) {
    if (accesses)
        fprintf(accesses, "mmu_rd_instr(memory, 0x%x);\n", addr);


    if (addr & 0b11)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    int result = cache_rd_instr(mem, addr);
    return result;
}

int mmu_rd_w(struct memory *mem, int addr) {
    if (accesses)
        fprintf(accesses, "mmu_rd_w(memory, 0x%x);\n", addr);

    if (addr & 0b11)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    int result = cache_rd_w(mem, addr);
    return result;
}

int mmu_rd_h(struct memory *mem, int addr) {
    if (accesses)
        fprintf(accesses, "mmu_rd_h(memory, 0x%x);\n", addr);
    //fprintf(accesses, "Reading a half from 0x%x\n", addr);

    if (addr & 0b1)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    int result = cache_rd_h(mem, addr);
    return result;
}

int mmu_rd_b(struct memory *mem, int addr) {
    if (accesses)
        fprintf(accesses, "mmu_rd_b(memory, 0x%x);\n", addr);
    
    int result = cache_rd_b(mem, addr);
    return result;
}

void dump_memory(uint32_t instr) {
    printf("Dumping cache state to dump.log");
    FILE* log = fopen("dump.log", "w");

    cache_dump_memory(log);
    
    fprintf(log, "Crashed on received instruction: %08x\n", instr);

    fclose(log);
}