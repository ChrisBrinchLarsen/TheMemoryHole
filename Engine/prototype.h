#pragma once

#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

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

    struct Cache* childCache;

    CacheLine_t **sets;

} Cache_t;

Cache_t* Cache_new(uint32_t cacheSize, uint32_t associativity) {
    Cache_t* c = malloc(sizeof(Cache_t));

    c->cacheSize = cacheSize;
    c->associativity = associativity;

    c->setCount = c->cacheSize / (c->associativity * BLOCK_SIZE);

    c->blockOffsetBitLength = log2(BLOCK_SIZE);

    c->SetBitLength = log2(c->setCount);
    c->TagBitLength = ADDR_LEN - c->blockOffsetBitLength - c->SetBitLength; // TODO ASSUMES VALID BIT LENGTH = 1 AND ADDRESS LENGTH = 32
    
    c->sets = malloc(c->setCount * sizeof(CacheLine_t*));
    for (uint32_t i = 0; i < c->setCount; i++) {
        c->sets[i] = malloc(c->associativity * sizeof(CacheLine_t));
        for (uint32_t j = 0; j < c->associativity; j++) {
            c->sets[i][j].valid = 0;
            c->sets[i][j].LRU = 0;
            c->sets[i][j].tag = 0;
            c->sets[i][j].block = 0; // NULL
        };
    };

    c->childCache = NULL;

    return c;
}

void Cache_free(Cache_t* c) {
    free(c);
}


void ReadMemory(Cache_t* cache, uint32_t address);
int IsLineInSet(Cache_t* cache, uint32_t setIndex, uint32_t tag);
void InsertLineInSet(Cache_t* cache, uint32_t setIndex, uint32_t tag);
void UpdateCacheSet(Cache_t* cache, uint32_t setIndex);

//void printBits(size_t const size, void const * const ptr);
void CacheSetToString(Cache_t* cache, int setIndex, char* out);
void CacheLineToString(Cache_t* cache, uint32_t setIndex, uint32_t lineIndex, char* out);
void PrintSet(Cache_t* cache, uint32_t setIndex);

Cache_t** ParseCPUArchitecture(char* path);