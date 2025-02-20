#include "mmu.h"
#include <stdio.h>
#include <stdint.h>

Cache_t* TOP_LEVEL_CACHE;

Cache_t* recieve_cache() {return TOP_LEVEL_CACHE;}



void supply_cache(Cache_t* cache) {
    TOP_LEVEL_CACHE = cache;
}



void memory_wr_w(struct memory *mem, int addr, uint32_t data) {
    FILE* accesses = fopen("accesses", "a");
    fprintf(accesses, "memory_wr_w(memory, 0x%x, %d);\n", addr, data);
    if (addr & 0b11)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    cache_wr_w(TOP_LEVEL_CACHE, mem, addr, data);
    print_all_caches(accesses);
    fclose(accesses);
}

void memory_wr_h(struct memory *mem, int addr, uint16_t data) {
    FILE* accesses = fopen("accesses", "a");
    fprintf(accesses, "memory_wr_h(memory, 0x%x, %d);\n", addr, data);
    if (addr & 0b1)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    cache_wr_h(TOP_LEVEL_CACHE, mem, addr, data);
    print_all_caches(accesses);
    fclose(accesses);
}

void memory_wr_b(struct memory *mem, int addr, uint8_t data) {
    FILE* accesses = fopen("accesses", "a");
    fprintf(accesses, "memory_wr_b(memory, 0x%x, %d);\n", addr, data);
    cache_wr_b(TOP_LEVEL_CACHE, mem, addr, data);
    print_all_caches(accesses);
    fclose(accesses);
}

int memory_rd_w(struct memory *mem, int addr) {
    FILE* accesses = fopen("accesses", "a");
    fprintf(accesses, "memory_rd_w(memory, 0x%x);\n", addr);
    if (addr & 0b11)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    int result = cache_rd_w(TOP_LEVEL_CACHE, mem, addr);
    print_all_caches(accesses);
    fclose(accesses);
    return result;
}

int memory_rd_h(struct memory *mem, int addr) {
    FILE* accesses = fopen("accesses", "a");
    fprintf(accesses, "memory_rd_h(memory, 0x%x);\n", addr);
    fprintf(accesses, "Reading a half from 0x%x\n", addr);
    if (addr & 0b1)
    {
        printf("Unaligned word write to %x\n", addr);
        exit(-1);
    }
    int result = cache_rd_h(TOP_LEVEL_CACHE, mem, addr);
    print_all_caches(accesses);
    fclose(accesses);
    return result;
}

int memory_rd_b(struct memory *mem, int addr) {
    FILE* accesses = fopen("accesses", "a");
    fprintf(accesses, "memory_rd_b(memory, 0x%x);\n", addr);
    int result = cache_rd_b(TOP_LEVEL_CACHE, mem, addr);
    print_all_caches(accesses);
    fclose(accesses);
    return result;
}

void print_all_caches(FILE* file) {
    Cache_t* cache = TOP_LEVEL_CACHE;
    int i = 1;
    do {
        PrintCache(cache);
        cache = cache->childCache;
        i++;
    } while (cache);
}