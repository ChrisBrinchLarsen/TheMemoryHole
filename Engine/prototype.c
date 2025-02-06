#include <stdio.h>  
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h> 
#include <stdint.h>
#include "prototype.h"


// Cache architecture
// const uint32_t CACHE_SIZE = 1024;          // Amount of bytes in cache
// #define BLOCK_SIZE 8                       // Amount of bytes in a block
// const uint32_t ASSOCIATIVITY = 4;           // How associative our cache is, determines how many lines are in each set
// const uint32_t ADDR_LEN = 32;               // The address length, usually 32-bit or 64-bit


// Policies
#define LRU_REPLACEMENT_POLICY 0
#define RANDOM_REPLACEMENT_POLICY 1
const uint32_t ACTIVE_REPLACEMENT_POLICY = LRU_REPLACEMENT_POLICY;

Cache_t* L1;

int main() {
    L1 = Cache_new(1024, 4, 8);

    ReadMemory(L1, 0b11111111111111111111111111111111);
    ReadMemory(L1, 0b11011111111111111111111111111111);
    ReadMemory(L1, 0b10111111111111111111111111111111);
    ReadMemory(L1, 0b10011111111111111111111111111111);

    // HIT test
    ReadMemory(L1, 0b11011111111111111111111111111111);

    // MISS
    ReadMemory(L1, 0b10001111111111111111111111111111);

    for (int i = 0; i < L1->setCount; i++) {
        PrintSet(L1, i);
    }

    
    return 0;
}

void ReadMemory(Cache_t* cache, uint32_t address) {

    /// separate the address to parts
    uint32_t blockOffset;
    uint32_t setIndex;
    uint32_t tag;
    uint32_t mask;
    
    // block offset
    mask = ~0;
    mask = mask << cache->blockOffsetBitLength;
    mask = ~mask;
    blockOffset = address & mask;

    // set index
    address = address >> cache->blockOffsetBitLength;
    mask = ~0;
    mask = mask << cache->SetBitLength;
    mask = ~mask;
    setIndex = address & mask;

    // tag
    address = address >> cache->SetBitLength;
    mask = ~0;
    mask = mask << cache->TagBitLength;
    mask = ~mask;
    tag = address & mask;

    UpdateCacheSet(cache, setIndex);

    // check if line is already in set, otherwise add it. CACHE HIT/MISS
    // MISS
    if (!IsLineInSet(cache, setIndex, tag)) {
        // count cache hits/misses
        InsertLineInSet(cache, setIndex, tag);
    }
    // HIT
    else {
        
    }
}


int IsLineInSet(Cache_t* cache, uint32_t setIndex, uint32_t tag) {
    for (uint32_t i = 0; i < cache->associativity; i++) {
        if (cache->sets[setIndex][i].tag == tag && cache->sets[setIndex][i].valid) {
            cache->sets[setIndex][i].LRU = 0; // least recently used; just now
            return 1;
        }
    }
    return 0;
}

void InsertLineInSet(Cache_t* cache, uint32_t setIndex, uint32_t tag) {

    // first check if there's room anywhere.
    int32_t insertIdx = -1;
    for (uint32_t i = 0; i < cache->associativity; i++) {
        if (!cache->sets[setIndex][i].valid) {
            insertIdx = i;
            break;
        }
    }

    // if no room, find room based on replacement policy
    if (insertIdx == -1) {
        
        switch (ACTIVE_REPLACEMENT_POLICY)
        {
            case LRU_REPLACEMENT_POLICY:
                {
                    uint32_t maxval = 0;
                    for (int i = 0; i < cache->associativity; i++) {
                        if (cache->sets[setIndex][i].LRU >= maxval) {
                            insertIdx = i;
                            maxval = cache->sets[setIndex][i].LRU;
                        }
                        
                    }
                }
                break;

            
            case RANDOM_REPLACEMENT_POLICY:
                insertIdx = 0;
                /* code */
                break;
            
            default:
                break;
        }
    }

    // writeback to lower cache

    // insert
    CacheLine_t c = CacheLine_new(1, tag, 0, NULL); // INSTEAD OF NULL POINTER, CALL L2

    cache->sets[setIndex][insertIdx] = c;
}

void UpdateCacheSet(Cache_t* cache, uint32_t setIndex) {
    for (int i = 0; i < cache->associativity; i++) {
        cache->sets[setIndex][i].LRU++;
    }
}

// tysm stackexchange (https://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format)
// void printBits(size_t const size, void const * const ptr) {
//     unsigned char *b = (unsigned char*) ptr;
//     unsigned char byte;
//     int i, j;
    
//     for (i = size-1; i >= 0; i--) {
//         for (j = 7; j >= 0; j--) {
//             byte = (b[i] >> j) & 1;
//             printf("%u", byte);
//         }
//     }
//     puts("");
// }


void CacheSetToString(Cache_t* cache, int setIndex, char* out) {
    char holder[400] = {0};

    for (int i = 0; i < cache->associativity; i++) {
        char buf[50] = {0};
        CacheLineToString(cache, setIndex, i, buf);
        strcat(holder, buf);
    }

    sprintf(out, "Set %02d |%s\n", setIndex, holder);
}

void CacheLineToString(Cache_t* cache, uint32_t setIndex, uint32_t lineIndex, char* out) {
    
    CacheLine_t cacheLine = cache->sets[setIndex][lineIndex];


    char* tag = malloc(1 + cache->TagBitLength * sizeof(char));
    for (uint32_t i = 0; i < cache->TagBitLength; i++) {
        int shft = (cacheLine.tag >> (cache->TagBitLength-i-1)) & 0b1;
        tag[i] = shft ? '1' : '0';
    }
    tag[cache->TagBitLength] = 0;

    sprintf(out, " (V:%d) (T:%s) (LRU:%d) |", cacheLine.valid, tag, cacheLine.LRU);
    
    free(tag);
}

void PrintSet(Cache_t* cache, uint32_t setIndex) {
    char buff[400] = {0};
    CacheSetToString(cache, setIndex, buff);
    printf(buff);
}