#include <stdio.h>  
#include <string.h>
// TODO : already included in the header file?
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "cache.h"
#include "mmu.h"



int ADDR_LEN = 32;
uint32_t N_CACHE_LEVELS;

Cache_t* caches;

FILE* CACHE_LOG;

// Policies
#define LRU_REPLACEMENT_POLICY 0
#define RANDOM_REPLACEMENT_POLICY 1
const uint32_t ACTIVE_REPLACEMENT_POLICY = LRU_REPLACEMENT_POLICY;


// Private function prototypes
void cache_writeback_block(uint32_t layer, int addr_int, char* data, size_t block_size);
char* fetch_block(uint32_t layer, uint32_t addr_int, struct memory *mem, bool mark_dirty);
void handle_miss(uint32_t layer, Address_t addr, struct memory* mem, bool mark_dirty);
void cache_writeback_block(uint32_t layer, int addr_int, char* data, size_t block_size);
int get_line_index_from_tag(Cache_t* cache, Address_t addr);
int get_replacement_line_index(uint32_t layer, uint32_t set_index);
void evict_cache_line(uint32_t layer, uint32_t addr_int, CacheLine_t* evict_line, struct memory *mem);
void invalidate_line(uint32_t layer, uint32_t addr_int);
void increment_line_LRU(Cache_t* cache, uint32_t setIndex);
void CacheSetToString(Cache_t* cache, int setIndex, char* out);
void CacheLineToString(Cache_t* cache, uint32_t setIndex, uint32_t lineIndex, char* out);
void PrintSet(Cache_t* cache, uint32_t setIndex);
void PrintCache(Cache_t* cache);
Cache_t* parse_cpu(char* path);
Address_t get_address(Cache_t* cache, uint32_t address);
Cache_t* cache_new(uint32_t cacheSize, uint32_t block_size, uint32_t associativity);
CacheLine_t cacheline_new(char* block);
void initialize_cache();
void finalize_cache();
FILE* get_cache_log();
int get_cache_layer_count();
uint64_t get_misses_at_layer(int layer);
uint64_t get_hits_at_layer(int layer);


void cache_wr_w(struct memory *mem, int addr_int, uint32_t data) {
    fprintf(CACHE_LOG, "ww 0x%x %d\n", addr_int, data);

    char* block = fetch_block(0, addr_int, mem, true);

    Address_t addr = get_address(&caches[0], addr_int);

    memcpy(&block[addr.block_offset], &data, sizeof(uint32_t));
}

void cache_wr_h(struct memory *mem, int addr_int, uint16_t data) {
    fprintf(CACHE_LOG, "wh 0x%x %d\n", addr_int, data);
    // TODO : Check if address in half-aligned
    char* block = fetch_block(0, addr_int, mem, true);

    Address_t addr = get_address(&caches[0], addr_int);

    memcpy(&block[addr.block_offset], &data, sizeof(uint16_t));
}

void cache_wr_b(struct memory *mem, int addr_int, uint8_t data) {
    fprintf(CACHE_LOG, "wb 0x%x %d\n", addr_int, data);
    char* block = fetch_block(0, addr_int, mem, true);

    Address_t addr = get_address(&caches[0], addr_int);

    memcpy(&block[addr.block_offset], &data, sizeof(uint8_t));
}

int cache_rd_w(struct memory *mem, int addr_int) {
    fprintf(CACHE_LOG, "rw 0x%x\n", addr_int);
    char* block = fetch_block(0, addr_int, mem, false);

    Address_t addr = get_address(&caches[0], addr_int);

    return (int)(*(uint32_t*)&block[addr.block_offset]);
}

int cache_rd_h(struct memory *mem, int addr_int) {
    fprintf(CACHE_LOG, "rh 0x%x\n", addr_int);
    char* block = fetch_block(0, addr_int, mem, false);

    Address_t addr = get_address(&caches[0], addr_int);

    return (int)(*(uint16_t*)&block[addr.block_offset]);
}

int cache_rd_b(struct memory *mem, int addr_int) {
    fprintf(CACHE_LOG, "rb 0x%x\n", addr_int);
    char* block = fetch_block(0, addr_int, mem, false);

    Address_t addr = get_address(&caches[0], addr_int);

    return (int)block[addr.block_offset];
}


char* fetch_block(uint32_t layer, uint32_t addr_int, struct memory *mem, bool mark_dirty) {
    Cache_t* cache = &caches[layer];
    Address_t addr = get_address(cache, addr_int);

    increment_line_LRU(cache, addr.set_index);

    // The index of the line that our address points to in the set our index points to
    int line_index = get_line_index_from_tag(cache, addr);

    // check if line is already in set, otherwise add it. CACHE HIT/MISS
    if (line_index == -1) { // MISS
        handle_miss(layer, addr, mem, mark_dirty);
    }
    // HIT
    else {
        // count cache hits
        cache->sets[addr.set_index][line_index].LRU = 0; // least recently used; just now
        cache->hits++;

        fprintf(CACHE_LOG, "H %d %d %d\n", cache->id, addr.set_index, line_index);
    }
    
    if (mark_dirty) {
        cache->sets[addr.set_index][line_index].dirty = true;
    }

    return cache->sets[addr.set_index][line_index].block;
}

void handle_miss(uint32_t layer, Address_t addr, struct memory* mem, bool mark_dirty) {
    Cache_t* cache = &caches[layer];
    fprintf(CACHE_LOG, "M %d %d\n", layer+1, addr.set_index);
    cache->misses++;

    int line_index = get_replacement_line_index(layer, addr.set_index);
    CacheLine_t* victim = &cache->sets[addr.set_index][line_index];
    if (victim->valid) {
        victim->valid = 0;
        invalidate_line(layer-1, addr.full_addr);
        if (victim->dirty) {evict_cache_line(layer, addr.full_addr, victim, mem);}
    }

    // the cache below returns its entire block, we need to know which part of that cache is our block (since it might be smaller)
    char* block = NULL;
    uint32_t block_offset = 0;

    if (layer != N_CACHE_LEVELS-1) { // Not at last layer of cache
        // recursive call
        block = fetch_block(layer+1, addr.full_addr, mem, mark_dirty);

        // in case were getting stuff from another layer of cache, we need to find the offset within the block we've been given

        // mask out the top and bottom bits. If L1 uses 00000011 and L2 uses 00001111, the block_idx after shifting and masking will be 00001100
        block_offset = (addr.full_addr >> cache->block_offset_bit_length) << cache->block_offset_bit_length;
        uint32_t mask = ((uint32_t)pow(2,(&caches[layer+1])->block_offset_bit_length)-1);
        block_offset = block_offset & mask;

    } else {
        fprintf(CACHE_LOG, "RAM\n");
        // when fetching from main memory, the block offset will just be 0, since we've already requested our specific size.
        block = find_block(mem, addr.full_addr, cache->block_size);
    }

    // Fetched a block into the cache
    fprintf(CACHE_LOG, "F %d %d %d\n", layer+1, addr.set_index, line_index);
    
    // Evict
    // if (victim->valid) {
    //     // Here we need to find the address of the cacheline to be able to find it in the lower cache, otherwise we don't know where to evict it to.
    //     uint32_t evictAddr = (cache->sets[a.setIndex][lineIndex].tag << (cache->SetBitLength + cache->block_offset_bit_length)) | (a.setIndex << cache->block_offset_bit_length);
    //     EvictCacheLine(cache, evictAddr, &cache->sets[a.setIndex][lineIndex], mem);
    // }


    // update line
    victim->valid = 1;
    victim->tag = addr.tag;
    victim->LRU = 0;
    // copy and insert block
    memcpy(victim->block, block, cache->block_size);
}

// NOTE: This function assumes that cache inclusivity holds
void cache_writeback_block(uint32_t layer, int addr_int, char* data, size_t block_size) {
    Cache_t* cache = &caches[layer];

    Address_t addr = get_address(cache, addr_int);

    // If inclusivity holds, we will always find a line in this set with a matching tag
    int line_index = get_line_index_from_tag(cache, addr);

    if (line_index == -1) {
        printf("ERROR: Tried to perform write-back, but block was missing in lower level cache. Cache inclusivity didn't hold.\n");
        PrintCache(cache);
        exit(1);
    }
    fprintf(CACHE_LOG, "E %d %d %d\n", layer+1, addr.set_index, line_index);

    char* block = cache->sets[addr.set_index][line_index].block;

    // TODO: Modify this such that we index in with the proper blockoffset in case the block sizes don't match
    memcpy(block, data, block_size);
}

int get_line_index_from_tag(Cache_t* cache, Address_t addr) {
    for (uint32_t i = 0; i < cache->associativity; i++) {
        if (cache->sets[addr.set_index][i].valid && cache->sets[addr.set_index][i].tag == addr.tag) {return i;}
    }
    return -1;
}

int get_replacement_line_index(uint32_t layer, uint32_t set_index) {
    CacheLine_t* set = caches[layer].sets[set_index];
    // first check if there's room anywhere.
    int line_index = -1;
    for (uint32_t i = 0; i < (&caches[layer])->associativity; i++) {
        if (!set[i].valid) {
            line_index = i;
            break;
        }
    }

    // if no room, find room based on replacement policy
    if (line_index == -1) {
        switch (ACTIVE_REPLACEMENT_POLICY) {
            case LRU_REPLACEMENT_POLICY:
                uint32_t maxval = 0;
                for (uint32_t i = 0; i < (&caches[layer])->associativity; i++) {
                    if (set[i].LRU >= maxval) {
                        line_index = i;
                        maxval = set[i].LRU;
                    }
                }
                break;
            case RANDOM_REPLACEMENT_POLICY:
                line_index = 0;
                break;
            default: break;
        }
    }

    if (line_index == -1) {
        printf("ERROR: Couldn't find line in L%d to mark as victim.\n", layer+1);
        exit(1);
    }

    return line_index;
}

void evict_cache_line(uint32_t layer, uint32_t addr_int, CacheLine_t* evict_line, struct memory *mem) {
    evict_line->dirty = false;
    if (layer == N_CACHE_LEVELS-1) { // Writing to main memory
        memory_write_back(mem, addr_int, evict_line->block, (&caches[layer])->block_size);
    } else { // Writing to cache
        cache_writeback_block(layer+1, addr_int, evict_line->block, (&caches[layer])->block_size);
    }
}

void invalidate_line(uint32_t layer, uint32_t addr_int) {
    Address_t addr = get_address(&caches[layer], addr_int);

    int line_index = get_line_index_from_tag(&caches[layer], addr);
    if (line_index != -1) {
        fprintf(CACHE_LOG, "I %d %d %d\n", layer+1, addr.set_index, line_index);
        (&caches[layer])->sets[addr.set_index][line_index].valid = 0;
        // Back-invalidation up the cache chain
        if (layer != 0) {invalidate_line(layer-1, addr_int);}
    }
}

void increment_line_LRU(Cache_t* cache, uint32_t setIndex) {
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


    char* tag = malloc(1 + cache->tag_bit_length * sizeof(char));
    for (uint32_t i = 0; i < cache->tag_bit_length; i++) {
        int shft = (cacheLine.tag >> (cache->tag_bit_length-i-1)) & 0b1;
        tag[i] = shft ? '1' : '0';
    }
    tag[cache->tag_bit_length] = 0;

    sprintf(out, " (V:%d) (T:%s) (LRU:%d) |", cacheLine.valid, tag, cacheLine.LRU);
    
    free(tag);
}

void PrintSet(Cache_t* cache, uint32_t setIndex) {
    char buff[400] = {0};
    CacheSetToString(cache, setIndex, buff);
    printf("%s", buff);
}

void PrintCache(Cache_t* cache) {
    for (uint32_t i = 0; i < cache->set_count; i++) {
        PrintSet(cache, i);
    }
}

Cache_t* parse_cpu(char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        printf("ERROR: Couldn't find file: '%s' when trying to parse a CPU architecture", path);
        exit(1);
    }
    char buf[32] = {0};
    fgets(buf, sizeof(buf), file);
    N_CACHE_LEVELS = (uint32_t)atoi(buf);
    memset(buf, 0, sizeof(buf));
    
    // Logging info about the general cache architecture
    caches = malloc(N_CACHE_LEVELS * (sizeof(Cache_t)));

    // Storage variables for:
    // Cache size: 2^p * q
    // Block size: 2^k
    // Associativity: a-way
    uint32_t p, q, k, a;

    for (uint32_t i = 0; i < N_CACHE_LEVELS; i++) {
        fgets(buf, sizeof(buf), file); // Name
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), file); // p
        p = atoi(buf);
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), file); // q
        q = atoi(buf);
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), file); // k
        k = atoi(buf);
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), file); // associativity
        a = atoi(buf);

        caches[i] = *cache_new((uint32_t)(pow(2,p) * q), (uint32_t)(pow(2,k)), a);
    }
    fclose(file);
    return caches;
}


Address_t get_address(Cache_t* cache, uint32_t address) {
    
    Address_t ad;
    ad.full_addr = address;

    uint32_t mask;
    // block offset
    mask = ~0;
    mask = mask << cache->block_offset_bit_length;
    mask = ~mask;
    ad.block_offset = address & mask;

    // set index
    address = address >> cache->block_offset_bit_length;
    mask = ~0;
    mask = mask << cache->set_bit_length;
    mask = ~mask;
    ad.set_index = address & mask;

    // tag
    address = address >> cache->set_bit_length;
    mask = ~0;
    mask = mask << cache->tag_bit_length;
    mask = ~mask;
    ad.tag = address & mask;

    return ad;
}


Cache_t* cache_new(uint32_t cacheSize, uint32_t block_size, uint32_t associativity) {
    Cache_t* c = (Cache_t*)malloc(sizeof(Cache_t));
    c->cache_size = cacheSize;
    c->block_size = block_size;
    c->associativity = associativity;
    c->set_count = (c->cache_size / (c->associativity * c->block_size));

    c->block_offset_bit_length = log2(c->block_size);
    c->set_bit_length = log2(c->set_count);
    c->tag_bit_length = ADDR_LEN - c->block_offset_bit_length - c->set_bit_length;

    c->hits = 0;
    c->misses = 0;
   
    c->sets = (CacheLine_t**)malloc(c->set_count * sizeof(CacheLine_t*));
    for (uint32_t i = 0; i < c->set_count; i++) {
        c->sets[i] = (CacheLine_t*)malloc(c->associativity * sizeof(CacheLine_t));
        for (uint32_t j = 0; j < c->associativity; j++) {
            c->sets[i][j] = cacheline_new(malloc(block_size * sizeof(char)));
        };
    };

    return c;
}

CacheLine_t cacheline_new(char* block) {
    CacheLine_t l;
    l.valid = 0;
    l.dirty = 0;
    l.LRU = 0;
    l.tag = 0;
    l.block = block;
    return l;
}

void initialize_cache() {
    CACHE_LOG = fopen("cache_log", "w");
}

// Returns amount of cycles spent on memory accesses
void finalize_cache() {
    fclose(CACHE_LOG);
}

FILE* get_cache_log() {return CACHE_LOG;}

int get_cache_layer_count() {return N_CACHE_LEVELS;}
uint64_t get_misses_at_layer(int layer) {return caches[layer].misses;}
uint64_t get_hits_at_layer(int layer) {return caches[layer].hits;}
