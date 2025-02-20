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
int N_CACHE_LEVELS;
int BLOCK_SIZE;

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

void cache_writeback_block(Cache_t *cache, int addr, char* data, size_t blockSize) {
    // NOTE: This function assumes that cache inclusivity holds

    printf("Began writing back to next layer of cache\n");

    Address_t a = GetAddress(cache, addr);
    int lineIndex = GetLineIndexFromTag(cache, a.setIndex, a.tag);

    if (lineIndex == -1) {
        printf("WARNING: cacheline in set 0x%x, lineindex 0x%x, (tag: 0x%x) was not found. Cache inclusivity might not've been held\n", a.setIndex, lineIndex, a.tag);
    }

    char* block = cache->sets[a.setIndex][lineIndex].block;

    // note right now this only works if we writeback to a cache with the same blocksize
    memcpy(block, data, blockSize);
    //memcpy(&block[a.blockOffset], &data, blockSize); // note blocksize was given from the cache above in case it's smaller
}

void cache_wr_w(Cache_t *cache, struct memory *mem, int addr, uint32_t data) {
    printf("Attempting to write a word to address 0x%x\n", addr);
    // TODO : Check if address is word-aligned
    char* block = FetchBlock(cache, addr, mem, true);

    Address_t a = GetAddress(cache, addr);

    memcpy(&block[a.blockOffset], &data, sizeof(uint32_t));
}

void cache_wr_h(Cache_t *cache, struct memory *mem, int addr, uint16_t data) {
    printf("Attempting to write a half to address 0x%x\n", addr);
    // TODO : Check if address in half-aligned
    char* block = FetchBlock(cache, addr, mem, true);

    Address_t a = GetAddress(cache, addr);

    memcpy(&block[a.blockOffset], &data, sizeof(uint16_t));
}

void cache_wr_b(Cache_t *cache, struct memory *mem, int addr, uint8_t data) {
    printf("Attempting to write a byte to address 0x%x\n", addr);
    char* block = FetchBlock(cache, addr, mem, true);

    Address_t a = GetAddress(cache, addr);

    memcpy(&block[a.blockOffset], &data, sizeof(uint8_t));
}

int cache_rd_w(Cache_t *cache, struct memory *mem, int addr) {
    printf("Attempting to read a word from address 0x%x\n", addr);
    char* block = FetchBlock(cache, addr, mem, false);

    Address_t a = GetAddress(cache, addr);

    return (int)(*(uint32_t*)&block[a.blockOffset]);
}

int cache_rd_h(Cache_t *cache, struct memory *mem, int addr) {
    printf("Attempting to read a half from address 0x%x\n", addr);
    char* block = FetchBlock(cache, addr, mem, false);

    Address_t a = GetAddress(cache, addr);

    return (int)(*(uint16_t*)&block[a.blockOffset]);
}

int cache_rd_b(Cache_t *cache, struct memory *mem, int addr) {
    printf("Attempting to read a byte from address 0x%x\n", addr);
    char* block = FetchBlock(cache, addr, mem, false);

    Address_t a = GetAddress(cache, addr);

    return (int)block[a.blockOffset];
}


// uint32_t getBlockOffset(Cache_t *cache, int addr) {
//     uint32_t mask;
//     uint32_t blockOffset;
//     mask = ~0;
//     mask = mask << cache->blockOffsetBitLength;
//     mask = ~mask;
//     blockOffset = addr & mask;
//     return blockOffset;
// }

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

char* FetchBlock(Cache_t* cache, uint32_t addr, struct memory *mem, bool markDirty) {

    printf("called FetchBlock on address 0x%x\n", addr);

    Address_t a = GetAddress(cache, addr);

    printf("tag: 0x%x\nsetIndex: 0x%x\nblockOffset: 0x%x\n", a.tag, a.setIndex, a.blockOffset);

    UpdateCacheSet(cache, a.setIndex);

    int lineIndex = GetLineIndexFromTag(cache, a.setIndex, a.tag);

    cache->sets[a.setIndex][lineIndex].LRU = 0; // least recently used; just now


    // check if line is already in set, otherwise add it. CACHE HIT/MISS
    // MISS
    if (lineIndex == -1) {
        printf("cache miss!\n");

        // the cache below returns its entire block, we need to know which part of that cache is our block (since it might be smaller)
        char* block = NULL;
        uint32_t blockidx = 0;

        if (cache->childCache != NULL) {
            // recursive call
            printf("calling FetchBlock on next layer of cache,\n");
            block = FetchBlock(cache->childCache, addr, mem, markDirty);

            // in case were getting stuff from another layer of cache, we need to find the offset within the block we've been given

            // mask out the top and bottom bits. If L1 uses 00000011 and L2 uses 00001111, the block_idx after shifting and masking will be 00001100
            blockidx = (addr >> cache->blockOffsetBitLength) << cache->blockOffsetBitLength;
            uint32_t mask = ((uint32_t)pow(2,cache->childCache->blockOffsetBitLength)-1);
            blockidx = blockidx & mask;

        }
        else {
            printf("no more cache layers. Calling find_block in main memory.\n");
            block = find_block(mem, addr, cache->blockSize);

            // when fetching from main memory, the blockidx will just be 0, since we've already requested our specific size.
        }

        printf("inserting cache line into cache\n");

        lineIndex = GetReplacementLineIndex(cache, a.setIndex);
        
        // evict
        if (cache->sets[a.setIndex][lineIndex].valid) {
            // Here we need to find the address of the cacheline to be able to find it in the lower cache, otherwise we don't know where to evict it to.
            printf("Tag: %x, SetIdx: %x\n", cache->sets[a.setIndex][lineIndex].tag, a.setIndex);
            uint32_t evictAddr = (cache->sets[a.setIndex][lineIndex].tag << (cache->SetBitLength + cache->blockOffsetBitLength)) | (a.setIndex << cache->blockOffsetBitLength);
            printf("Evict address: 0x%x\n", evictAddr);
            EvictCacheLine(cache, evictAddr, &cache->sets[a.setIndex][lineIndex], mem);
        }

        // update line
        cache->sets[a.setIndex][lineIndex].valid = 1;
        cache->sets[a.setIndex][lineIndex].tag = a.tag;
        cache->sets[a.setIndex][lineIndex].LRU = 0;
        // copy and insert block
        memcpy(cache->sets[a.setIndex][lineIndex].block, &block[blockidx], cache->blockSize); // block_size * word_size????? idk

    }
    // HIT
    else {
        // count cache hits
        printf("cache hit!\n");
    }
    
    if (markDirty) {
        printf("marking cache line as dirty\n");
        cache->sets[a.setIndex][lineIndex].dirty = true;
    }

    printf("returning block to previous cache layer.\n");

    return cache->sets[a.setIndex][lineIndex].block;
}

int GetLineIndexFromTag(Cache_t* cache, uint32_t setIndex, uint32_t tag) {
    for (uint32_t i = 0; i < cache->associativity; i++) {
        if (cache->sets[setIndex][i].tag == tag && cache->sets[setIndex][i].valid) {
            return i;
        }
    }
    return -1;
}

int GetReplacementLineIndex(Cache_t* cache, uint32_t setIndex) {

    printf("finding line index\n");
    // first check if there's room anywhere.
    int32_t lineIndex = -1;
    for (uint32_t i = 0; i < cache->associativity; i++) {
        if (!cache->sets[setIndex][i].valid) {
            lineIndex = i;
            break;
        }
    }

    // if no room, find room based on replacement policy
    if (lineIndex == -1) {
        printf("cache is full, another line needs to be evicted\n");
        switch (ACTIVE_REPLACEMENT_POLICY)
        {
            case LRU_REPLACEMENT_POLICY:
                {
                    uint32_t maxval = 0;
                    for (uint32_t i = 0; i < cache->associativity; i++) {
                        if (cache->sets[setIndex][i].LRU >= maxval) {
                            lineIndex = i;
                            maxval = cache->sets[setIndex][i].LRU;
                        }
                        
                    }
                }
                break;

            
            case RANDOM_REPLACEMENT_POLICY:
                lineIndex = 0;
                /* code */
                break;
            
            default:
                break;
        }
    }

    // insert
    //CacheLine_t c = CacheLine_new(1, tag, 0, block);
    printf("lineIndex: %u\n", lineIndex);
    return lineIndex;
}

void EvictCacheLine(Cache_t* cache, uint32_t addr, CacheLine_t* evict_line, struct memory *mem) {
    printf("evicting cache line.\n");

    if (evict_line->dirty) {

        if (cache->childCache == NULL) {
            printf("writing back to main memory.\n");
            memory_write_back(mem, addr, evict_line->block, cache->blockSize);
        } else {
            printf("writing back to next layer of cache.\n");
            cache_writeback_block(cache->childCache, addr, evict_line->block, cache->blockSize);
        }
    }

    // dunno if this is needed, but it probably saves us from some headaches. Maybe not super accurate to real life though
    evict_line->valid = false;
    evict_line->dirty = false;
}


// TODO : better name
void UpdateCacheSet(Cache_t* cache, uint32_t setIndex) {
    for (uint32_t i = 0; i < cache->associativity; i++) {
        cache->sets[setIndex][i].LRU++;
    }
}

void CacheSetToString(Cache_t* cache, int setIndex, char* out) {
    char holder[400] = {0};

    for (uint32_t i = 0; i < cache->associativity; i++) {
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
    for (uint32_t i = 0; i < cache->setCount; i++) {
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
    N_CACHE_LEVELS = atoi(buf);
    memset(buf, 0, sizeof(buf));
    
    printf("During architecture parsing, we just finished reading globals\n");

    Cache_t** caches = malloc(N_CACHE_LEVELS * (sizeof(Cache_t*)));

    for (int i = 0; i < N_CACHE_LEVELS; i++) {
        // size = 2^p * q
        // block_size = 2^k
        fgets(buf, sizeof(buf), file); // Name
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), file); // p
        uint32_t p = atoi(buf);
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), file); // q
        uint32_t q = atoi(buf);
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), file); // k
        uint32_t k = atoi(buf);
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), file); // associativity
        uint32_t associativity = atoi(buf);


        BLOCK_SIZE = (int)pow(2, k);
        uint32_t cache_size = (uint32_t)(pow(2,p) * q);
        caches[i] = Cache_new(cache_size, associativity);
    }

    fclose(file);
    printf("We just finished reading info from the file\n");

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

Address_t GetAddress(Cache_t* cache, uint32_t address) {
    
    Address_t ad;
    ad.fullAddr = address;

    uint32_t mask;
    // block offset
    mask = ~0;
    mask = mask << cache->blockOffsetBitLength;
    mask = ~mask;
    ad.blockOffset = address & mask;

    // set index
    address = address >> cache->blockOffsetBitLength;
    mask = ~0;
    mask = mask << cache->SetBitLength;
    mask = ~mask;
    ad.setIndex = address & mask;

    // tag
    address = address >> cache->SetBitLength;
    mask = ~0;
    mask = mask << cache->TagBitLength;
    mask = ~mask;
    ad.tag = address & mask;

    return ad;
}


Cache_t* Cache_new(uint32_t cacheSize, uint32_t associativity) {
    Cache_t* c = (Cache_t*)malloc(sizeof(Cache_t));

    c->cacheSize = cacheSize;
    c->associativity = associativity;

    c->setCount = (c->cacheSize / (c->associativity * BLOCK_SIZE));

    c->blockOffsetBitLength = log2(BLOCK_SIZE);

    c->SetBitLength = log2(c->setCount);
    c->TagBitLength = ADDR_LEN - c->blockOffsetBitLength - c->SetBitLength; // TODO ASSUMES VALID BIT LENGTH = 1 AND ADDRESS LENGTH = 32
    
    c->blockSize = BLOCK_SIZE;

    // TODO : Call CacheLine_t constructor 
    c->sets = (CacheLine_t**)malloc(c->setCount * sizeof(CacheLine_t*));
    for (uint32_t i = 0; i < c->setCount; i++) {
        c->sets[i] = (CacheLine_t*)malloc(c->associativity * sizeof(CacheLine_t));
        for (uint32_t j = 0; j < c->associativity; j++) {
            c->sets[i][j].valid = 0;
            c->sets[i][j].dirty = 0;
            c->sets[i][j].LRU = 0;
            c->sets[i][j].tag = 0;
            c->sets[i][j].block = (char*)malloc(BLOCK_SIZE * sizeof(char)); // NULL
        };
    };

    c->childCache = NULL;

    return c;
}

void Cache_free(Cache_t* c) {
    free(c);
}

CacheLine_t CacheLine_new(bool valid, bool dirty, uint32_t tag, uint32_t LRU, char* block) {
    CacheLine_t l;
    l.valid = valid;
    l.dirty = dirty;
    l.tag = tag;
    l.LRU = LRU;
    l.block = block;
    return l;
}

void CacheLine_free(CacheLine_t* l) {
    free(l->block);
    free(l);
}