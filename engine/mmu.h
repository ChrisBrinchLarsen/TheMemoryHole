#ifndef __MMU_H__
#define __MMU_H__

#include <stdint.h>
#include "cache.h"
#include <stdio.h>

Cache_t* recieve_cache();
void supply_cache(Cache_t* cache);

// skriv word/halfword/byte til lager
void memory_wr_w(struct memory *mem, int addr, uint32_t data);
void memory_wr_h(struct memory *mem, int addr, uint16_t data);
void memory_wr_b(struct memory *mem, int addr, uint8_t data);

// læs word/halfword/byte fra lager - data er nul-forlænget
int memory_rd_w(struct memory *mem, int addr);
int memory_rd_h(struct memory *mem, int addr);
int memory_rd_b(struct memory *mem, int addr);

#endif