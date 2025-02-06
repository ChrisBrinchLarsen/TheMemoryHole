#include <stdio.h>  
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h> 
#include <stdint.h>
#include "prototype.h"


// Cache architecture
const uint32_t CACHE_SIZE = 32768;          // Amount of bytes in cache
#define BLOCK_SIZE 16                       // Amount of bytes in a block
const uint32_t ASSOCIATIVITY = 2;           // How associative our cache is, determines how many lines are in each set
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
const uint32_t ACTIVE_REPLACEMENT_POLICY = RANDOM_REPLACEMENT_POLICY;

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
            L1[i][j].tag = 0;
        };
    };

    //ReadMemory(0b10001100011110100111001100110001);
    ReadMemory(0b11111111111111111111111111111111);
    
    return 0;
}

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
    if (!IsLineInSet(L1[setIndex], tag)) {
        // count cache hits/misses
        InsertLineInSet(L1[setIndex], tag);
        char buff[500] = {0};
        CacheSetToString(L1, setIndex, buff);
        printf(buff);
    }
    else {
        UpdateCacheSet(L1[setIndex]);
    }

}


int IsLineInSet(CacheLine_t *set, uint32_t tag) {
    for (uint32_t i = 0; i < ASSOCIATIVITY; i++) {
        if (set[i].tag == tag && set[i].valid)
            return 1;
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
    char holder[50 * ASSOCIATIVITY] = {0};
    CacheLine_t* lines = Cache[setIndex];

    for (int i = 0; i < ASSOCIATIVITY; i++) {
        char buf[50] = {0};
        CacheLineToString(lines[i], buf);
        strcat(holder, buf);
    }

    sprintf(out, "Set %d |%s\n", setIndex, holder);
}

void CacheLineToString(CacheLine_t cacheLine, char* out) {
    
    char valid = cacheLine.valid ? '1' : '0';

    char* tag = malloc(TAG_BIT_LENGTH * sizeof(char));
    for (uint32_t i = 0; i < TAG_BIT_LENGTH; i++) {
        int shft = (cacheLine.tag >> (TAG_BIT_LENGTH-i-1)) & 0b1;
        tag[i] = shft ? '1' : '0';
    }

    sprintf(out, " (V:%s) (T:%s) (LRU:%c) |", &valid, tag, 'x');
    
    free(tag);
}