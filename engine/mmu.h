#ifndef __MMU_H__
#define __MMU_H__

#include <stdint.h>
#include "cache.h"
#include <stdio.h>

Cache_t* recieve_cache();
void supply_cache(Cache_t* cache);

void open_accesses_file();
void close_accesses_file();

uint32_t mmu_get_checksum();

// skriv word/halfword/byte til lager
void mmu_wr_w_instr(struct memory *mem, int addr, uint32_t data);
void mmu_wr_h_instr(struct memory *mem, int addr, uint16_t data);
void mmu_wr_b_instr(struct memory *mem, int addr, uint8_t data);

// skriv word/halfword/byte til lager
void mmu_wr_w(struct memory *mem, int addr, uint32_t data);
void mmu_wr_h(struct memory *mem, int addr, uint16_t data);
void mmu_wr_b(struct memory *mem, int addr, uint8_t data);

// læs word/halfword/byte fra lager - data er nul-forlænget
int mmu_rd_instr(struct memory *mem, int addr);
int mmu_rd_w(struct memory *mem, int addr);
int mmu_rd_h(struct memory *mem, int addr);
int mmu_rd_b(struct memory *mem, int addr);

void dump_memory();

#endif