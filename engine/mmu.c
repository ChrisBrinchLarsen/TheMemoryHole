#include "mmu.h"
#include <stdint.h>
#include "cache.h"


void memory_wr_w(struct memory *mem, int addr, uint32_t data) {
    cache_wr_w(TOP_LEVEL_CACHE, mem, addr, data);
}

void memory_wr_h(struct memory *mem, int addr, uint16_t data) {
    cache_wr_h(TOP_LEVEL_CACHE, mem, addr, data);
}

void memory_wr_b(struct memory *mem, int addr, uint8_t data) {
    cache_wr_b(TOP_LEVEL_CACHE, mem, addr, data);
}

int memory_rd_w(struct memory *mem, int addr) {
    return cache_rd_w(TOP_LEVEL_CACHE, mem, addr);
}

int memory_rd_h(struct memory *mem, int addr) {
    return cache_rd_h(TOP_LEVEL_CACHE, mem, addr);
}

int memory_rd_b(struct memory *mem, int addr) {
    return cache_rd_b(TOP_LEVEL_CACHE, mem, addr);
}