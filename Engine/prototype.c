#include <stdio.h>  
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h> 
#include <stdint.h>
#include "prototype.h"


// Cache architecture
const uint32_t CACHE_SIZE = 1024;          // Amount of bytes in cache
#define BLOCK_SIZE 8                       // Amount of bytes in a block
const uint32_t ASSOCIATIVITY = 4;           // How associative our cache is, determines how many lines are in each set
const uint32_t ADDR_LEN = 32;               // The address length, usually 32-bit or 64-bit

// Address bit fields
uint32_t BLOCK_OFFSET_BIT_LENGTH;
uint32_t SET_COUNT;
uint32_t SET_BIT_LENGTH;
uint32_t VALID_BIT_LENGTH;
uint32_t TAG_BIT_LENGTH;


// Policies
#define LRU_REPLACEMENT_POLICY 0
#define RANDOM_REPLACEMENT_POLICY 1
const uint32_t ACTIVE_REPLACEMENT_POLICY = LRU_REPLACEMENT_POLICY;

CacheLine_t** L1;

int main() {
    // Initializing address bit fields
    BLOCK_OFFSET_BIT_LENGTH = log2(BLOCK_SIZE);
    SET_COUNT = CACHE_SIZE / (ASSOCIATIVITY * BLOCK_SIZE);
    SET_BIT_LENGTH = log2(SET_COUNT);
    VALID_BIT_LENGTH = 1;
    TAG_BIT_LENGTH = ADDR_LEN - BLOCK_OFFSET_BIT_LENGTH - SET_BIT_LENGTH - VALID_BIT_LENGTH;
    
    // Malloc 2D array of all sets with all of their lines
    L1 = malloc(SET_COUNT * sizeof(CacheLine_t *));
    for (uint32_t i = 0; i < SET_COUNT; i++) {
        L1[i] = malloc(ASSOCIATIVITY * sizeof(CacheLine_t));
        for (uint32_t j = 0; j < ASSOCIATIVITY; j++) {
            L1[i][j].valid = 0;
            L1[i][j].LRU = 0;
            L1[i][j].tag = 0;
        };
    };

    //ReadMemory(0b10001100011110100111001100110001);
    ReadMemory(0b11111111111111111111111111111111);
    ReadMemory(0b11011111111111111111111111111111);
    ReadMemory(0b10111111111111111111111111111111);
    ReadMemory(0b10011111111111111111111111111111);
    
    // HIT test
    ReadMemory(0b11011111111111111111111111111111);

    // MISS
    ReadMemory(0b10001111111111111111111111111111);

    for (int i = 0; i < SET_COUNT; i++) {
        PrintSet(i);
    }

    
    return 0;
}

void ReadMemory(uint32_t address) {

    /// separate the address to parts
    uint32_t blockOffset;
    uint32_t setIndex;
    uint32_t tag;
    uint32_t mask;
    
    // block offset
    mask = ~0;
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
    mask = ~0;
    mask = mask << TAG_BIT_LENGTH;
    mask = ~mask;
    tag = address & mask;

    UpdateCacheSet(L1[setIndex]);

    // check if line is already in set, otherwise add it. CACHE HIT/MISS
    // MISS
    if (!IsLineInSet(L1[setIndex], tag)) {
        // count cache hits/misses
        InsertLineInSet(L1[setIndex], tag);
    }
    // HIT
    else {
        
    }
}


int IsLineInSet(CacheLine_t *set, uint32_t tag) {
    for (uint32_t i = 0; i < ASSOCIATIVITY; i++) {
        if (set[i].tag == tag && set[i].valid) {
            set[i].LRU = 0;
            return 1;
        }
    }
    return 0;
}

void InsertLineInSet(CacheLine_t *set, uint32_t tag) {

    // first check if there's room anywhere.
    int32_t insertIdx = -1;
    for (uint32_t i = 0; i < ASSOCIATIVITY; i++) {
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
                {
                    uint32_t maxval = 0;
                    for (int i = 0; i < ASSOCIATIVITY; i++) {
                        if (set[i].LRU >= maxval) {
                            insertIdx = i;
                            maxval = set[i].LRU;
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
    CacheLine_t c;
    c.valid = 1;
    c.LRU = 0;
    c.tag = tag;
    //memcpy(&c.block, blockData, BLOCK_SIZE * sizeof(char)); // CALL TO L2
    set[insertIdx] = c;
}

void UpdateCacheSet(CacheLine_t *set) {
    for (int i = 0; i < ASSOCIATIVITY; i++) {
        set[i].LRU++;
    }
}

// tysm stackexchange (https://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format)
void printBits(size_t const size, void const * const ptr) {
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


void CacheSetToString(CacheLine_t** Cache, int setIndex, char* out) {
    char holder[400] = {0};
    CacheLine_t* lines = Cache[setIndex];

    for (int i = 0; i < ASSOCIATIVITY; i++) {
        char buf[50] = {0};
        CacheLineToString(lines[i], buf);
        strcat(holder, buf);
    }

    sprintf(out, "Set %02d |%s\n", setIndex, holder);
}

void CacheLineToString(CacheLine_t cacheLine, char* out) {
    
    char valid = cacheLine.valid ? '1' : '0';
    

    char* tag = malloc(1 + TAG_BIT_LENGTH * sizeof(char));
    for (uint32_t i = 0; i < TAG_BIT_LENGTH; i++) {
        int shft = (cacheLine.tag >> (TAG_BIT_LENGTH-i-1)) & 0b1;
        tag[i] = shft ? '1' : '0';
    }
    tag[TAG_BIT_LENGTH] = 0;

    sprintf(out, " (V:%s) (T:%s) (LRU:%d) |", &valid, tag, cacheLine.LRU);
    
    free(tag);
}

void PrintSet(uint32_t setIndex) {
    char buff[400] = {0};
    CacheSetToString(L1, setIndex, buff);
    printf(buff);
}