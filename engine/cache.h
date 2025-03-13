#ifndef __CACHE_H__
#define __CACHE_H__

#include "memory.h"
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct CacheLine {
    bool valid;
    bool dirty;
    uint32_t tag;
    uint32_t LRU;
    char* block;
} CacheLine_t;

typedef struct Cache {
    //
    uint32_t id;

    // base parameters
    uint32_t cacheSize;
    uint32_t associativity;

    uint32_t setCount;

    // bit sizes
    uint32_t blockOffsetBitLength;
    uint32_t SetBitLength;
    uint32_t TagBitLength;

    uint32_t blockSize;

    struct Cache* childCache;

    CacheLine_t **sets;

} Cache_t;

Cache_t** ParseCPUArchitecture(char* path);

// skriv word/halfword/byte til lager
void cache_wr_w(Cache_t *cache, struct memory *mem, int addr, uint32_t data);
void cache_wr_h(Cache_t *cache, struct memory *mem, int addr, uint16_t data);
void cache_wr_b(Cache_t *cache, struct memory *mem, int addr, uint8_t data);

// læs word/halfword/byte fra lager - data er nul-forlænget
int cache_rd_w(Cache_t *cache, struct memory *mem, int addr);
int cache_rd_h(Cache_t *cache, struct memory *mem, int addr);
int cache_rd_b(Cache_t *cache, struct memory *mem, int addr);

int get_cache_layer_count();
uint64_t get_misses_at_layer(int layer);
uint64_t get_hits_at_layer(int layer);

void initialize_cache();
uint64_t finalize_cache();
FILE* get_cache_log();

void PrintSet(Cache_t* cache, uint32_t setIndex);
void PrintCache(Cache_t* cache);

#endif