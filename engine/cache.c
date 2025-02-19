#include <stdio.h>  
#include <string.h>
#include "cache.h"

// TODO : already included in the header file?
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

// Cache architecture
// const uint32_t CACHE_SIZE = 1024;          // Amount of bytes in cache
// #define BLOCK_SIZE 8                       // Amount of bytes in a block
// const uint32_t ASSOCIATIVITY = 4;           // How associative our cache is, determines how many lines are in each set
// const uint32_t ADDR_LEN = 32;               // The address length, usually 32-bit or 64-bit

// shouldn't need to be exposed actually
int ADDR_LEN;
int WORD_SIZE;
int BLOCK_SIZE;
int N_CACHE_LEVELS;

// Policies
#define LRU_REPLACEMENT_POLICY 0
#define RANDOM_REPLACEMENT_POLICY 1
const uint32_t ACTIVE_REPLACEMENT_POLICY = LRU_REPLACEMENT_POLICY;

Cache_t* L1;

void init_cache(int argc, char** argv) {

    // We perform unit tests if no additional arguments are provided
    if (argc == 1) {
        Cache_t** caches = ParseCPUArchitecture("./testing/Architectures/SimpleCPU.md");
        L1 = caches[0];
        printf("L1:\n");
        for (uint32_t i = 0; i < L1->setCount; i++) {
            PrintSet(L1, i);
        }
        printf("L2:\n");
        for (uint32_t i = 0; i < L1->childCache->setCount; i++) {
            PrintSet(L1->childCache, i);
        }
        
        //ParseMemoryRequests("./testing/Instructions/Simple.md");

        printf("L1:\n");
        for (uint32_t i = 0; i < L1->setCount; i++) {
            PrintSet(L1, i);
        }
        printf("L2:\n");
        for (uint32_t i = 0; i < L1->childCache->setCount; i++) {
            PrintSet(L1->childCache, i);
        }
        return;
    }

    Cache_t** caches = ParseCPUArchitecture(argv[1]);
    //ParseMemoryRequests(argv[2]);

    L1 = caches[0];
    
    return;
}


void cache_wr_w(Cache_t *cache, int addr, uint32_t data) {

}

void cache_wr_h(Cache_t *cache, int addr, uint16_t data) {
}

void cache_wr_b(Cache_t *cache, int addr, uint8_t data) {
}

int cache_rd_w(Cache_t *cache, struct memory *mem, int addr) {
    char* block = FetchBlock(cache, addr, mem);

    uint32_t blockOffset = getBlockOffset(cache, addr);

    return (int)(*(uint32_t*)&block[blockOffset]);

}

int cache_rd_h(Cache_t *cache, struct memory *mem, int addr) {
    char* block = FetchBlock(cache, addr, mem);

    uint32_t blockOffset = getBlockOffset(cache, addr);

    return (int)(*(uint16_t*)&block[blockOffset]);
}

int cache_rd_b(Cache_t *cache, struct memory *mem, int addr) {
    char* block = FetchBlock(cache, addr, mem);

    uint32_t blockOffset = getBlockOffset(cache, addr);

    return (int)block[blockOffset];
}


uint32_t getBlockOffset(Cache_t *cache, int addr) {
    uint32_t mask;
    uint32_t blockOffset;
    mask = ~0;
    mask = mask << cache->blockOffsetBitLength;
    mask = ~mask;
    blockOffset = addr & mask;
    return blockOffset;
}

// char ReadData(Cache_t* cache, uint32_t address) {
//     char* block = FetchBlock(cache, address);

//     uint32_t mask;
//     uint32_t blockOffset;
//     mask = ~0;
//     mask = mask << cache->blockOffsetBitLength;
//     mask = ~mask;
//     blockOffset = address & mask;

//     return block[blockOffset * WORD_SIZE];
// }

char* FetchBlock(Cache_t* cache, uint32_t addr, struct memory *mem) {

    printf("called FetchBlock on address %x\n", addr);
    /// separate the address to parts
    uint32_t blockOffset;
    uint32_t setIndex;
    uint32_t tag;
    uint32_t mask;

    // block offset
    mask = ~0;
    mask = mask << cache->blockOffsetBitLength;
    mask = ~mask;
    blockOffset = addr & mask;

    // set index
    addr = addr >> cache->blockOffsetBitLength;
    mask = ~0;
    mask = mask << cache->SetBitLength;
    mask = ~mask;
    setIndex = addr & mask;

    // tag
    addr = addr >> cache->SetBitLength;
    mask = ~0;
    mask = mask << cache->TagBitLength;
    mask = ~mask;
    tag = addr & mask;

    printf("tag: %x\nsetIndex: %x\nblockOffset: %x\n", tag, setIndex, blockOffset);

    UpdateCacheSet(cache, setIndex);

    int lineIndex = GetLineIndexFromTag(cache, setIndex, tag);

    // check if line is already in set, otherwise add it. CACHE HIT/MISS
    // MISS
    if (lineIndex == -1) {
        printf("cache miss!\n");

        char* block = NULL;

        // count cache misses

        if (cache->childCache != NULL) {
            // recursive call
            printf("calling FetchBlock on next layer of cache,\n");
            block = FetchBlock(cache->childCache, addr, mem);

        }
        else { // TODO : fetch block from main memory
            printf("no more cache layers. Calling find_block in main memory.");
            block = find_block(mem, addr, cache->blockSize);
        }

        printf("inserting cache line into cache");

        // the cache below returns its entire block, we need to know which part of that cache is our block (since it might be smaller)
        uint32_t blockidx = addr;
        // mask out the top and bottom bits. If L1 uses 00000011 and L2 uses 00001111, the block_idx after shifting and masking will be 00001100
        blockidx = (blockidx >> cache->blockOffsetBitLength) << cache->blockOffsetBitLength;
        uint32_t mask = ((uint32_t)pow(2,cache->childCache->blockOffsetBitLength)-1);
        blockidx = blockidx & mask;

        // copy and insert block
        InsertLineInSet(cache, setIndex, tag, &block[blockidx]);
    }
    // HIT
    else {
        // count cache hits
        printf("cache hit!");
    }
    
    printf("returning block to previous cache layer");
    return cache->sets[setIndex][lineIndex].block;
}

int GetLineIndexFromTag(Cache_t* cache, uint32_t setIndex, uint32_t tag) {
    for (uint32_t i = 0; i < cache->associativity; i++) {
        if (cache->sets[setIndex][i].tag == tag && cache->sets[setIndex][i].valid) {
            cache->sets[setIndex][i].LRU = 0; // least recently used; just now
            return i;
        }
    }
    return -1;
}

void InsertLineInSet(Cache_t* cache, uint32_t setIndex, uint32_t tag, char* block) {

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
                    for (uint32_t i = 0; i < cache->associativity; i++) {
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
    //CacheLine_t c = CacheLine_new(1, tag, 0, block);

    cache->sets[setIndex][insertIdx].valid = 1;
    cache->sets[setIndex][insertIdx].tag = tag;
    cache->sets[setIndex][insertIdx].LRU = 0;
    memcpy(cache->sets[setIndex][insertIdx].block, block, cache->blockSize); // block_size * word_size????? idk

}

// TODO : better name
void UpdateCacheSet(Cache_t* cache, uint32_t setIndex) {
    for (int i = 0; i < cache->associativity; i++) {
        cache->sets[setIndex][i].LRU++;
    }
}

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
void PrintCache(Cache_t* cache) {
    for (uint32_t i = 0; i < L1->setCount; i++) {
        PrintSet(cache, i);
    }
}


Cache_t** ParseCPUArchitecture(char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        printf("ERROR: Couldn't find file: '%s' when trying to parse a CPU architecture", path);
        exit(1);
    }
    char buf[64] = {0};
    fgets(buf, sizeof(buf), file);
    ADDR_LEN = atoi(buf);
    memset(buf, 0, sizeof(buf));

    fgets(buf, sizeof(buf), file);
    WORD_SIZE = atoi(buf);
    memset(buf, 0, sizeof(buf));

    fgets(buf, sizeof(buf), file);
    BLOCK_SIZE = atoi(buf);
    memset(buf, 0, sizeof(buf));

    fgets(buf, sizeof(buf), file);
    N_CACHE_LEVELS = atoi(buf);
    memset(buf, 0, sizeof(buf));
    
    Cache_t** caches = malloc(N_CACHE_LEVELS * (sizeof(Cache_t*)));

    for (int i = 0; i < N_CACHE_LEVELS; i++) {
        fgets(buf, sizeof(buf), file); // Name
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), file); // Size
        uint32_t size = atoi(buf);
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), file); // Associativity
        uint32_t associativity = atoi(buf);

        caches[i] = Cache_new(size, associativity);
    }

    fclose(file);

    for (int i = 0; i < (N_CACHE_LEVELS-1); i++) {
        caches[i]->childCache = caches[i+1]; // L1 -> L2 -> L3 -> NULL (since children are set to NULL in constructor)
    }

    return caches;
}

// void ParseMemoryRequests(char* path) {
//     FILE* file = fopen(path, "r");
//     if (!file) {
//         printf("ERROR: Couldn't find file: '%s' when trying to parse a file of memory requests", path);
//         exit(1);
//     }

//     char buf[65] = {0};
//     while (fgets(buf, sizeof(buf), file)) {
//         char type = buf[ADDR_LEN];

//         ReadData(L1, BinStrToNum(buf, ADDR_LEN));
//     }
// }

// Takes in a binary string, computes the value as an unsigned 64-bit integer
// The input for n should usually be either 32 or 64
uint64_t BinStrToNum(char* num, int n) {
    uint64_t result = 0;
    for (int i = 0; i < n; i++) {
        result += pow(2, i) * (num[n - i - 1] - '0');
    }
    return result;
}


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