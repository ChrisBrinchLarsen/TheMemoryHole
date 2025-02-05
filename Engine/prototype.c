#include <stdio.h>  
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h> 
#include <stdint.h>

// Cache architecture
const uint32_t CACHE_SIZE = 32768;          // Amount of bytes in cache
#define BLOCK_SIZE 16                       // Amount of bytes in a block
const uint32_t ASSOCIATIVITY = 2;           // How associative our cache is, determines how many lines are in each set
const uint32_t ADDR_LEN = 32;               // The address length, usually 32-bit or 64-bit

// Address bit fields
uint32_t BLOCK_OFFSET_BIT_LENGTH;
uint32_t SET_BIT_LENGTH;
uint32_t VALID_BIT_LENGTH;
uint32_t TAG_BIT_LENGTH;

// Policies
#define LRU_REPLACEMENT_POLICY 0
#define RANDOM_REPLACEMENT_POLICY 1
const uint32_t ACTIVE_REPLACEMENT_POLICY = RANDOM_REPLACEMENT_POLICY;


uint32_t linesPerSet = 8; // det her er også mega temporary


typedef struct CacheLine {
    bool valid;
    uint32_t tag;
    char block[BLOCK_SIZE];
} CacheLine_t;

CacheLine_t** L1Cache;

void ReadMemory(uint32_t address) {

    /// separate the address to parts
    uint32_t blockOffset;
    uint32_t setIndex;
    uint32_t tag;

    // block offset
    uint32_t mask = ~0;
    mask = mask << BLOCK_OFFSET_BIT_LENGTH;
    mask = ~mask;
    blockOffset = address & mask;

    // set index
    address = address >> BLOCK_OFFSET_BIT_LENGTH;
    mask = ~0;
    mask = mask << SET_BIT_LENGTH;
    mask = ~mask;
    setIndex = address & mask;

    // tag
    address = address >> SET_BIT_LENGTH;
    tag = address;


    // check if line is already in set, otherwise add it. CACHE HIT/MISS
    if (!IsLineInSet(L1Cache[setIndex], tag)) {
        // count cache hits/misses
        InsertLineInSet(L1Cache[setIndex], tag);
    }
    else {
        UpdateCacheSet(L1Cache[setIndex]);
    }

}

void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}



int IsLineInSet(CacheLine_t *set, uint32_t tag) {
    for (uint32_t i = 0; i < linesPerSet; i++) {
        if (set[i].tag == tag && set[i].valid)
            return 1;
    }
    return 0;
}



void InsertLineInSet(CacheLine_t *set, uint32_t tag) {

    // first check if there's room anywhere.
    uint32_t insertIdx = -1;
    for (uint32_t i = 0; i < linesPerSet; i++) {
        if (!set[i].valid) {
            insertIdx = i;
            break;
        }
    }

    // if no room, find room based on replacement policy
    if (insertIdx == -1) {
        switch (ACTIVE_REPLACEMENT_POLICY)
        {
            case LRU_REPLACEMENT_POLICY:
                insertIdx = 0;
                /* code */
                break;
            
            case RANDOM_REPLACEMENT_POLICY:
                insertIdx = 0;
                /* code */
                break;
            
            default:
                break;
        }
    }

    // insert
    CacheLine_t c;
    c.valid = 1;
    c.tag = tag;
    //memcpy(&c.block, blockData, BLOCK_SIZE * sizeof(char)); // CALL TO L2
    set[insertIdx] = c;

    UpdateCacheSet(set);
}

void UpdateCacheSet(CacheLine_t *set) {

}

int main() {
    // Initializing address bit fields
    BLOCK_OFFSET_BIT_LENGTH = log2(BLOCK_SIZE);
    SET_BIT_LENGTH = log2(CACHE_SIZE / (ASSOCIATIVITY * BLOCK_SIZE));
    VALID_BIT_LENGTH = 1;
    TAG_BIT_LENGTH = ADDR_LEN - BLOCK_OFFSET_BIT_LENGTH - SET_BIT_LENGTH - VALID_BIT_LENGTH;

    uint32_t setCount = pow(2, SET_BIT_LENGTH);
    uint32_t blocksPerSet = pow(2, BLOCK_OFFSET_BIT_LENGTH);

    // malloc 2d array af alle cache lines
    L1Cache = malloc(setCount * linesPerSet * sizeof(intptr_t));
    for (uint32_t i = 0; i < setCount; i++) {
        L1Cache[i] = malloc(linesPerSet * sizeof(CacheLine_t));
    }

    ReadMemory(0b10001100011110100111001100110001);

    return 0;
}