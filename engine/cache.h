#ifndef __CACHE_H__
#define __CACHE_H__

#include "memory.h"
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <math.h>








// shouldn't need to be exposed actually
int ADDR_LEN;
int WORD_SIZE;
int BLOCK_SIZE;
int N_CACHE_LEVELS;

typedef struct CacheLine {
    bool valid;
    uint32_t tag;
    uint32_t LRU;
    char* block;
} CacheLine_t;

CacheLine_t CacheLine_new(bool valid, uint32_t tag, uint32_t LRU, char* block) {
    CacheLine_t l;
    l.valid = valid;
    l.tag = tag;
    l.LRU = LRU;
    l.block = block;
    return l;
}
void CacheLine_free(CacheLine_t* l) {
    free(l->block);
    free(l);
}

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

Cache_t* Cache_new(uint32_t cacheSize, uint32_t associativity) {
    Cache_t* c = (Cache_t*)(sizeof(Cache_t));

    c->cacheSize = cacheSize;
    c->associativity = associativity;

    c->setCount = c->cacheSize / (c->associativity * BLOCK_SIZE);

    c->blockOffsetBitLength = log2(BLOCK_SIZE);

    c->SetBitLength = log2(c->setCount);
    c->TagBitLength = ADDR_LEN - c->blockOffsetBitLength - c->SetBitLength; // TODO ASSUMES VALID BIT LENGTH = 1 AND ADDRESS LENGTH = 32
    
    c->blockSize = BLOCK_SIZE;

    // TODO : Call CacheLine_t constructor 
    c->sets = (CacheLine_t**)(c->setCount * sizeof(CacheLine_t*));
    for (uint32_t i = 0; i < c->setCount; i++) {
        c->sets[i] = (CacheLine_t*)malloc(c->associativity * sizeof(CacheLine_t));
        for (uint32_t j = 0; j < c->associativity; j++) {
            c->sets[i][j].valid = 0;
            c->sets[i][j].LRU = 0;
            c->sets[i][j].tag = 0;
            c->sets[i][j].block = (char*)malloc(BLOCK_SIZE * WORD_SIZE * sizeof(char)); // NULL
        };
    };

    c->childCache = NULL;

    return c;
}

void Cache_free(Cache_t* c) {
    free(c);
}

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