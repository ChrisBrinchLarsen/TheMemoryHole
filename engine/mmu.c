#include "mmu.h"
#include "memory.h"
#include <stdio.h>
#include <stdint.h>

Cache_t* TOP_LEVEL_CACHE;
struct memory* mem;
FILE* accesses;

Cache_t* recieve_cache() {return TOP_LEVEL_CACHE;}
void supply_cache(Cache_t* cache) {
    TOP_LEVEL_CACHE = cache;
}
void open_accesses_file() {
    accesses = fopen("accesses", "a");
}
void close_accesses_file() {
    fclose(accesses);
}

void mmu_wr_w_instr(struct memory *mem, int addr, uint32_t data) {
    
    fprintf(accesses, "mmu_wr_w_instr(memory, 0x%x, %d);\n", addr, data);
    if (addr & 0b11)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    memory_wr_w(mem, addr, data);
}

void mmu_wr_h_instr(struct memory *mem, int addr, uint16_t data) {
    fprintf(accesses, "mmu_wr_h_instr(memory, 0x%x, %d);\n", addr, data);
    if (addr & 0b1)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    memory_wr_h(mem, addr, data);
}

void mmu_wr_b_instr(struct memory *mem, int addr, uint8_t data) {
    fprintf(accesses, "mmu_wr_b_instr(memory, 0x%x, %d);\n", addr, data);
    memory_wr_b(mem, addr, data);
}

void mmu_wr_w(struct memory *mem, int addr, uint32_t data) {
    fprintf(accesses, "mmu_wr_w(memory, 0x%x, %d);\n", addr, data);
    if (addr & 0b11)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    cache_wr_w(TOP_LEVEL_CACHE, mem, addr, data);
}

void mmu_wr_h(struct memory *mem, int addr, uint16_t data) {
    fprintf(accesses, "mmu_wr_h(memory, 0x%x, %d);\n", addr, data);
    if (addr & 0b1)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    cache_wr_h(TOP_LEVEL_CACHE, mem, addr, data);
}

void mmu_wr_b(struct memory *mem, int addr, uint8_t data) {
    FILE* accesses = fopen("accesses", "a");
    fprintf(accesses, "mmu_wr_b(memory, 0x%x, %d);\n", addr, data);
    cache_wr_b(TOP_LEVEL_CACHE, mem, addr, data);
}

int mmu_rd_w(struct memory *mem, int addr) {
    FILE* accesses = fopen("accesses", "a");
    fprintf(accesses, "mmu_rd_w(memory, 0x%x);\n", addr);
    if (addr & 0b11)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    int result = cache_rd_w(TOP_LEVEL_CACHE, mem, addr);
    return result;
}

int mmu_rd_h(struct memory *mem, int addr) {
    fprintf(accesses, "mmu_rd_h(memory, 0x%x);\n", addr);
    fprintf(accesses, "Reading a half from 0x%x\n", addr);
    if (addr & 0b1)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    int result = cache_rd_h(TOP_LEVEL_CACHE, mem, addr);
    return result;
}

int mmu_rd_b(struct memory *mem, int addr) {
    fprintf(accesses, "mmu_rd_b(memory, 0x%x);\n", addr);
    int result = cache_rd_b(TOP_LEVEL_CACHE, mem, addr);
    return result;
}