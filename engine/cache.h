#ifndef __CACHE_H__
#define __CACHE_H__

#include "memory.h"
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef struct CacheLine {
    bool valid;
    uint32_t tag;
    uint32_t LRU;
    char* block;
} CacheLine_t;

CacheLine_t CacheLine_new(bool valid, uint32_t tag, uint32_t LRU, char* block);

void CacheLine_free(CacheLine_t* l);

typedef struct Cache {
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

Cache_t* Cache_new(uint32_t cacheSize, uint32_t associativity);

void Cache_free(Cache_t* c);

void init_cache(int argc, char** argv);
uint32_t getBlockOffset(Cache_t *cache, int addr);
//char ReadData(Cache_t* cache, uint32_t address);
char* FetchBlock(Cache_t* cache, uint32_t addr, struct memory *mem);
int GetLineIndexFromTag(Cache_t* cache, uint32_t setIndex, uint32_t tag);
void InsertLineInSet(Cache_t* cache, uint32_t setIndex, uint32_t tag, char* block);
void UpdateCacheSet(Cache_t* cache, uint32_t setIndex);

//void printBits(size_t const size, void const * const ptr);
void CacheSetToString(Cache_t* cache, int setIndex, char* out);
void CacheLineToString(Cache_t* cache, uint32_t setIndex, uint32_t lineIndex, char* out);
void PrintSet(Cache_t* cache, uint32_t setIndex);
void PrintCache(Cache_t* cache);

uint64_t BinStrToNum(char* num, int n);
void ParseMemoryRequests(char* path);

Cache_t** ParseCPUArchitecture(char* path);


// skriv word/halfword/byte til lager
void cache_wr_w(Cache_t *cache, int addr, uint32_t data);
void cache_wr_h(Cache_t *cache, int addr, uint16_t data);
void cache_wr_b(Cache_t *cache, int addr, uint8_t data);

// læs word/halfword/byte fra lager - data er nul-forlænget
int cache_rd_w(Cache_t *cache, struct memory *mem, int addr);
int cache_rd_h(Cache_t *cache, struct memory *mem, int addr);
int cache_rd_b(Cache_t *cache, struct memory *mem, int addr);

#endif