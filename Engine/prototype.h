#pragma once

#include <stdbool.h> 
#include <stdint.h>

#define BLOCK_SIZE 16
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
    uint32_t blockSize;

    uint32_t setCount;

    // bit sizes
    uint32_t blockOffsetBitLength;
    uint32_t SetBitLength;
    uint32_t TagBitLength;

    CacheLine_t **sets;

} Cache_t;

Cache_t* Cache_new(uint32_t cacheSize, uint32_t associativity, uint32_t blockSize) {
    Cache_t* c = malloc(sizeof(Cache_t));

    c->cacheSize = cacheSize;
    c->associativity = associativity;
    c->blockSize = blockSize;

    c->setCount = c->cacheSize / (c->associativity * c->blockSize);

    c->blockOffsetBitLength = log2(blockSize);

    c->SetBitLength = log2(c->setCount);
    c->TagBitLength = 32 - c->blockOffsetBitLength - c->SetBitLength - 1; // TODO ASSUMES VALID BIT LENGTH = 1 AND ADDRESS LENGTH = 32
    
    c->sets = malloc(c->setCount * sizeof(CacheLine_t*));
    for (uint32_t i = 0; i < c->setCount; i++) {
        c->sets[i] = malloc(c->associativity * sizeof(CacheLine_t));
        for (uint32_t j = 0; j < c->associativity; j++) {
            c->sets[i][j].valid = 0;
            c->sets[i][j].LRU = 0;
            c->sets[i][j].tag = 0;
        };
    };
    return c;
}
void Cache_free(Cache_t* c) {
    free(c);
}


void ReadMemory(uint32_t address);
int IsLineInSet(CacheLine_t *set, uint32_t tag);
void InsertLineInSet(CacheLine_t *set, uint32_t tag);
void UpdateCacheSet(CacheLine_t *set);
void CacheSetToString(CacheLine_t** Cache, int setIndex, char* out);
void CacheLineToString(CacheLine_t cacheLine, char* out);