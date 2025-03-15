#ifndef __CACHE_H__
#define __CACHE_H__

#include "memory.h"
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct Address {
    uint32_t full_addr;
    uint32_t tag;
    uint32_t set_index;
    uint32_t block_offset;
} Address_t;

typedef struct CacheLine {
    bool valid;
    bool dirty;
    uint32_t tag;
    uint32_t LRU;
    char* block;
} CacheLine_t;

typedef struct Cache {
    uint32_t id;

    // base parameters
    uint32_t cache_size;
    uint32_t associativity;

    uint32_t set_count;

    // bit sizes
    uint32_t block_offset_bit_length;
    uint32_t set_bit_length;
    uint32_t tag_bit_length;

    uint32_t block_size;

    CacheLine_t **sets;

    uint64_t hits;
    uint64_t misses;

} Cache_t;

Cache_t* parse_cpu(char* path);

// skriv word/halfword/byte til lager
void cache_wr_w(struct memory *mem, int addr, uint32_t data);
void cache_wr_h(struct memory *mem, int addr, uint16_t data);
void cache_wr_b(struct memory *mem, int addr, uint8_t data);

// læs word/halfword/byte fra lager - data er nul-forlænget
int cache_rd_w(struct memory *mem, int addr);
int cache_rd_h(struct memory *mem, int addr);
int cache_rd_b(struct memory *mem, int addr);

int get_cache_layer_count();
uint64_t get_misses_at_layer(int layer);
uint64_t get_hits_at_layer(int layer);

void initialize_cache();
void finalize_cache();
FILE* get_cache_log();

<<<<<<< HEAD
void print_all_caches();
=======
void PrintSet(Cache_t* cache, uint32_t setIndex);
void PrintCache(Cache_t* cache);
>>>>>>> master

#endif