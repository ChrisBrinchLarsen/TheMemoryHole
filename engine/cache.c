#include <stdio.h>  
#include <string.h>
#include "cache.h"

// TODO : already included in the header file?
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "mmu.h"

typedef struct Address {
    uint32_t fullAddr;
    uint32_t tag;
    uint32_t setIndex;
    uint32_t blockOffset;
} Address_t;

// shouldn't need to be exposed actually
int ADDR_LEN = 32;
int N_CACHE_LEVELS;
int BLOCK_SIZE;

// Some premade set clock cycle delays for hitting
const int HIT_DELAYS[4] = {2, 10, 20, 50};
const int RAM_DELAY = 100;
uint32_t CYCLES = 0;

// if misses[0] = 20 that means that the first layer of cache experienced 20 misses
uint32_t* MISSES;
// if hits[2] = 14 that means that the third layer of cache experienced 14 misses
uint32_t* HITS;

FILE* CACHE_LOG;

// Policies
#define LRU_REPLACEMENT_POLICY 0
#define RANDOM_REPLACEMENT_POLICY 1
const uint32_t ACTIVE_REPLACEMENT_POLICY = LRU_REPLACEMENT_POLICY;

Cache_t* L1;

// Private function prototypes
char* FetchBlock(Cache_t* cache, uint32_t addr, struct memory *mem, bool markDirty, int layer);
int GetLineIndexFromTag(Cache_t* cache, uint32_t setIndex, uint32_t tag);
int GetReplacementLineIndex(Cache_t* cache, uint32_t setIndex);
void UpdateCacheSet(Cache_t* cache, uint32_t setIndex);
void EvictCacheLine(Cache_t* cache, uint32_t addr, CacheLine_t* evict_line, struct memory *mem, int layer);
void CacheSetToString(Cache_t* cache, int setIndex, char* out);
void CacheLineToString(Cache_t* cache, uint32_t setIndex, uint32_t lineIndex, char* out);
void PrintSet(Cache_t* cache, uint32_t setIndex);
void PrintCache(Cache_t* cache);
void cache_writeback_block(Cache_t *cache, int addr, char* data, size_t blockSize, int layer);
Address_t GetAddress(Cache_t* cache, uint32_t address);
void Cache_free(Cache_t* c);
uint64_t BinStrToNum(char* num, int n);
Cache_t* Cache_new(uint32_t cacheSize, uint32_t associativity);
CacheLine_t CacheLine_new(bool valid, bool dirty, uint32_t tag, uint32_t LRU, char* block);
void CacheLine_free(CacheLine_t* l);


void cache_writeback_block(Cache_t *cache, int addr, char* data, size_t blockSize, int layer) {
    // NOTE: This function assumes that cache inclusivity holds

    Address_t a = GetAddress(cache, addr);
    int lineIndex = GetLineIndexFromTag(cache, a.setIndex, a.tag);
    fprintf(CACHE_LOG, "%d E %d %d\n", layer, a.setIndex, lineIndex);

    if (lineIndex == -1) {
        printf("ERROR: cacheline in set 0x%x, lineindex 0x%x, (tag: 0x%x) was not found. Cache inclusivity might not've been held\n", a.setIndex, lineIndex, a.tag);
        PrintCache(recieve_cache());
        PrintCache(cache);
        exit(-1);
    }

    char* block = cache->sets[a.setIndex][lineIndex].block;

    // note right now this only works if we writeback to a cache with the same blocksize
    memcpy(block, data, blockSize);
    //memcpy(&block[a.blockOffset], &data, blockSize); // note blocksize was given from the cache above in case it's smaller
}

void cache_wr_w(Cache_t *cache, struct memory *mem, int addr, uint32_t data) {
    fprintf(CACHE_LOG, "ww 0x%x %d\n", addr, data);

    // TODO : Check if address is word-aligned
    char* block = FetchBlock(cache, addr, mem, true, 1);

    Address_t a = GetAddress(cache, addr);

    memcpy(&block[a.blockOffset], &data, sizeof(uint32_t));
}

void cache_wr_h(Cache_t *cache, struct memory *mem, int addr, uint16_t data) {
    fprintf(CACHE_LOG, "wh 0x%x %d\n", addr, data);
    // TODO : Check if address in half-aligned
    char* block = FetchBlock(cache, addr, mem, true, 1);

    Address_t a = GetAddress(cache, addr);

    memcpy(&block[a.blockOffset], &data, sizeof(uint16_t));
}

void cache_wr_b(Cache_t *cache, struct memory *mem, int addr, uint8_t data) {
    fprintf(CACHE_LOG, "wb 0x%x %d\n", addr, data);
    char* block = FetchBlock(cache, addr, mem, true, 1);

    Address_t a = GetAddress(cache, addr);

    memcpy(&block[a.blockOffset], &data, sizeof(uint8_t));
}

int cache_rd_w(Cache_t *cache, struct memory *mem, int addr) {
    fprintf(CACHE_LOG, "rw 0x%x\n", addr);
    char* block = FetchBlock(cache, addr, mem, false, 1);

    Address_t a = GetAddress(cache, addr);

    return (int)(*(uint32_t*)&block[a.blockOffset]);
}

int cache_rd_h(Cache_t *cache, struct memory *mem, int addr) {
    fprintf(CACHE_LOG, "rh 0x%x\n", addr);
    char* block = FetchBlock(cache, addr, mem, false, 1);

    Address_t a = GetAddress(cache, addr);

    return (int)(*(uint16_t*)&block[a.blockOffset]);
}

int cache_rd_b(Cache_t *cache, struct memory *mem, int addr) {
    fprintf(CACHE_LOG, "rb 0x%x\n", addr);
    char* block = FetchBlock(cache, addr, mem, false, 1);

    Address_t a = GetAddress(cache, addr);

    return (int)block[a.blockOffset];
}


char* FetchBlock(Cache_t* cache, uint32_t addr, struct memory *mem, bool markDirty, int layer) {

    Address_t a = GetAddress(cache, addr);

    //printf("Tag: 0x%x\nSet Index: 0x%x\nBlock Offset: 0x%x\n", a.tag, a.setIndex, a.blockOffset);

    UpdateCacheSet(cache, a.setIndex); // Updating LRU fields

    int lineIndex = GetLineIndexFromTag(cache, a.setIndex, a.tag);

    // check if line is already in set, otherwise add it. CACHE HIT/MISS
    // MISS
    if (lineIndex == -1) {
        fprintf(CACHE_LOG, "%d M %d\n", layer, a.setIndex);
        MISSES[layer-1]++;

        // the cache below returns its entire block, we need to know which part of that cache is our block (since it might be smaller)
        char* block = NULL;
        uint32_t blockidx = 0;

        if (cache->childCache != NULL) {
            // recursive call
            block = FetchBlock(cache->childCache, addr, mem, markDirty, layer+1);

            // in case were getting stuff from another layer of cache, we need to find the offset within the block we've been given

            // mask out the top and bottom bits. If L1 uses 00000011 and L2 uses 00001111, the block_idx after shifting and masking will be 00001100
            blockidx = (addr >> cache->blockOffsetBitLength) << cache->blockOffsetBitLength;
            uint32_t mask = ((uint32_t)pow(2,cache->childCache->blockOffsetBitLength)-1);
            blockidx = blockidx & mask;

        }
        else {
            fprintf(CACHE_LOG, "RAM\n");
            CYCLES += RAM_DELAY;
            block = find_block(mem, addr, cache->blockSize);

            // when fetching from main memory, the blockidx will just be 0, since we've already requested our specific size.
        }

        lineIndex = GetReplacementLineIndex(cache, a.setIndex);
        fprintf(CACHE_LOG, "%d I %d %d\n", layer, a.setIndex, lineIndex);
        
        // evict
        if (cache->sets[a.setIndex][lineIndex].valid) {
            // Here we need to find the address of the cacheline to be able to find it in the lower cache, otherwise we don't know where to evict it to.
            uint32_t evictAddr = (cache->sets[a.setIndex][lineIndex].tag << (cache->SetBitLength + cache->blockOffsetBitLength)) | (a.setIndex << cache->blockOffsetBitLength);
            EvictCacheLine(cache, evictAddr, &cache->sets[a.setIndex][lineIndex], mem, layer);
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
        cache->sets[a.setIndex][lineIndex].LRU = 0; // least recently used; just now
        HITS[layer-1]++;
        CYCLES += HIT_DELAYS[layer-1];

        fprintf(CACHE_LOG, "%d H %d %d\n", layer, a.setIndex, lineIndex);
    }
    
    if (markDirty) {
        cache->sets[a.setIndex][lineIndex].dirty = true;
    }

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
    return lineIndex;
}

void EvictCacheLine(Cache_t* cache, uint32_t addr, CacheLine_t* evict_line, struct memory *mem, int layer) {

    if (evict_line->dirty) {

        if (cache->childCache == NULL) { // Writing to main memory
            memory_write_back(mem, addr, evict_line->block, cache->blockSize);
        } else { // Writing to cache
            cache_writeback_block(cache->childCache, addr, evict_line->block, cache->blockSize, layer+1);
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
    printf("%s", buff);
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
    N_CACHE_LEVELS = atoi(buf);
    memset(buf, 0, sizeof(buf));
    HITS =   malloc(N_CACHE_LEVELS * sizeof(uint32_t));
    MISSES = malloc(N_CACHE_LEVELS * sizeof(uint32_t));
    
    // Logging info about the general cache architecture
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

    for (int i = 0; i < (N_CACHE_LEVELS-1); i++) {
        caches[i]->childCache = caches[i+1]; // L1 -> L2 -> L3 -> NULL (since children are set to NULL in constructor)
    }
    return caches;
}

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

void initialize_cache() {
    CACHE_LOG = fopen("cache_log", "w");
}

// Returns amount of cycles spent on memory accesses
uint32_t finalize_cache() {
    fclose(CACHE_LOG);
    return CYCLES;
}

FILE* get_cache_log() {return CACHE_LOG;}

int get_cache_layer_count() {return N_CACHE_LEVELS;}
int get_misses_at_layer(int layer) {return MISSES[layer];}
int get_hits_at_layer(int layer) {return HITS[layer];}
