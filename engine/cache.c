#include <stdio.h>  
#include <string.h>
// TODO : already included in the header file?
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "cache.h"
#include "mmu.h"

uint32_t cache_checksum;

int ADDR_LEN = 32;
uint32_t N_CACHE_LEVELS;

Cache_t* caches = NULL;
Cache_t* L1i = NULL;

Cache_t* get_caches() { return caches; };
Cache_t* get_l1i() { return L1i; };

FILE* CACHE_LOG;

// Policies
#define LRU_REPLACEMENT_POLICY 0
#define RANDOM_REPLACEMENT_POLICY 1
const uint32_t ACTIVE_REPLACEMENT_POLICY = LRU_REPLACEMENT_POLICY;


// Private function prototypes
void cache_writeback_block(Cache_t* cache, int addr_int, char* data, size_t block_size);
CacheLine_t* fetch_line(Cache_t* cache, uint32_t addr_int, struct memory *mem);
void handle_miss(Cache_t* cache, Address_t addr, struct memory* mem);
int get_line_index_from_tag(Cache_t* cache, Address_t addr);
int get_replacement_line_index(Cache_t* cache, uint32_t set_index);
void evict_cache_line(Cache_t* cache, uint32_t addr_int, CacheLine_t* evict_line, struct memory *mem);
void invalidate_line(Cache_t* cache, uint32_t addr_int, struct memory* mem);
void increment_line_LRU(Cache_t* cache, uint32_t setIndex);
void CacheSetToString(Cache_t* cache, int setIndex, char* out);
void CacheLineToString(Cache_t* cache, uint32_t setIndex, uint32_t lineIndex, char* out);
void PrintSet(Cache_t* cache, uint32_t setIndex);
void PrintCache(Cache_t* cache);
Address_t get_address(Cache_t* cache, uint32_t address);
Cache_t* cache_new(uint32_t layer, uint32_t cacheSize, uint32_t block_size, uint32_t associativity);
CacheLine_t cacheline_new(char* block);
void change_validity(Cache_t* cache, int set_index, int line_index, bool new_validity);
void change_dirtiness(Cache_t* cache, int set_index, int line_index, bool new_dirty);

void add_operation_to_checksum(uint32_t type, uint32_t cache_idx, uint32_t set_idx, uint32_t line_idx);


void cache_wr_w(struct memory *mem, int addr_int, uint32_t data) {
    fprintf(CACHE_LOG, "ww 0x%x %d\n", addr_int, data);

    CacheLine_t* line = fetch_line(&caches[0], addr_int, mem);

    Address_t addr = get_address(&caches[0], addr_int);
    change_dirtiness(&caches[0],
                     addr.set_index,
                     get_line_index_from_tag(&caches[0], addr),
                     1);


    memcpy(&line->block[addr.block_offset], &data, sizeof(uint32_t));
}

void cache_wr_h(struct memory *mem, int addr_int, uint16_t data) {
    fprintf(CACHE_LOG, "wh 0x%x %d\n", addr_int, data);
    // TODO : Check if address in half-aligned
    CacheLine_t* line = fetch_line(&caches[0], addr_int, mem);
    
    Address_t addr = get_address(&caches[0], addr_int);
    change_dirtiness(&caches[0],
                     addr.set_index,
                     get_line_index_from_tag(&caches[0], addr),
                     1);


    memcpy(&line->block[addr.block_offset], &data, sizeof(uint16_t));
}

void cache_wr_b(struct memory *mem, int addr_int, uint8_t data) {
    fprintf(CACHE_LOG, "wb 0x%x %d\n", addr_int, data);
    
    CacheLine_t* line = fetch_line(&caches[0], addr_int, mem);
    Address_t addr = get_address(&caches[0], addr_int);
    change_dirtiness(&caches[0],
                     addr.set_index,
                     get_line_index_from_tag(&caches[0], addr),
                     1);

    memcpy(&line->block[addr.block_offset], &data, sizeof(uint8_t));
}

int cache_rd_instr(struct memory *mem, int addr_int) {
    if (L1i == NULL) { // in case we aren't using an instruction cache, we simply redirect this to the normal cache_rd_w method instead
        return cache_rd_w(mem, addr_int);
    }

    fprintf(CACHE_LOG, "rw 0x%x\n", addr_int); // TODO is this logging correct?
    char* block = fetch_line(L1i, addr_int, mem)->block;

    Address_t addr = get_address(L1i, addr_int);

    return (int)(*(uint32_t*)&block[addr.block_offset]);
}


int cache_rd_w(struct memory *mem, int addr_int) {
    fprintf(CACHE_LOG, "rw 0x%x\n", addr_int);
    char* block = fetch_line(&caches[0], addr_int, mem)->block;

    Address_t addr = get_address(&caches[0], addr_int);

    return (int)(*(uint32_t*)&block[addr.block_offset]);
}

int cache_rd_h(struct memory *mem, int addr_int) {
    fprintf(CACHE_LOG, "rh 0x%x\n", addr_int);
    char* block = fetch_line(&caches[0], addr_int, mem)->block;

    Address_t addr = get_address(&caches[0], addr_int);

    return (int)(*(uint16_t*)&block[addr.block_offset]);
}

int cache_rd_b(struct memory *mem, int addr_int) {
    fprintf(CACHE_LOG, "rb 0x%x\n", addr_int);
    char* block = fetch_line(&caches[0], addr_int, mem)->block;

    Address_t addr = get_address(&caches[0], addr_int);

    return (int)block[addr.block_offset];
}


/// @brief recursive function that finds a block, starting at some Layer. Updates caches accordingly
/// @param layer the layer of the cache
/// @param addr_int address
/// @param mem memory
/// @param mark_dirty whether to mark things dirty (in case of a write) 
/// @return 
CacheLine_t* fetch_line(Cache_t* cache, uint32_t addr_int, struct memory *mem) {
    Address_t addr = get_address(cache, addr_int);

    increment_line_LRU(cache, addr.set_index);

    // The index of the line that our address points to in the set our index points to
    int line_index = get_line_index_from_tag(cache, addr);

    // check if line is already in set, otherwise add it. CACHE HIT/MISS
    if (line_index == -1) { // MISS
        line_index = get_replacement_line_index(cache, addr.set_index);
        handle_miss(cache, addr, mem);
    }
    else { // HIT
        cache->sets[addr.set_index][line_index].LRU = 0; // least recently used; just now
        
        fprintf(CACHE_LOG, "H %d %d %d\n", cache->layer+1, addr.set_index, line_index);
    }

    return &(cache->sets[addr.set_index][line_index]);
}

void handle_miss(Cache_t* cache, Address_t addr, struct memory* mem) {
    fprintf(CACHE_LOG, "M %d %d\n", cache->layer+1, addr.set_index);

    // replace, back-invalidate and evict another line in this set first.
    int line_index = get_replacement_line_index(cache, addr.set_index);
    CacheLine_t* victim = &cache->sets[addr.set_index][line_index];
    if (victim->valid) { // sometimes the line to replace is unused, in which case we don't need to invalidate or evict anything
        if (cache->layer != 0) { //back invalidation
            uint32_t inval_line_addr = (victim->tag << (cache->set_bit_length + cache->block_offset_bit_length)) | (addr.set_index << cache->block_offset_bit_length);
            invalidate_line(cache->parent_cache, inval_line_addr, mem); // back invalidation request 
        }
        if (victim->dirty) {
            uint32_t evict_addr = (victim->tag << (cache->set_bit_length + cache->block_offset_bit_length)) | (addr.set_index << cache->block_offset_bit_length);
            evict_cache_line(cache, evict_addr, victim, mem);
        }
    }

    // the cache below returns its entire block, we need to know which part of that cache is our block (since it might be smaller)
    CacheLine_t* received_cache_line_from_child = NULL;
    char* block;
    uint32_t block_offset = 0;
    if (!(cache->child_cache == NULL)) { // If not at last layer of cache, request from lower cache
        // recursive call
        received_cache_line_from_child = fetch_line(cache->child_cache, addr.full_addr, mem);
        block = received_cache_line_from_child->block; // TODO fjern block var idk

        // in case were getting stuff from another layer of cache, we need to find the offset within the block we've been given

        // mask out the top and bottom bits. If L1 uses 00000011 and L2 uses 00001111, the block_idx after shifting and masking will be 00001100
        block_offset = (addr.full_addr >> cache->block_offset_bit_length) << cache->block_offset_bit_length;
        uint32_t mask = ((uint32_t)pow(2,caches->child_cache->block_offset_bit_length)-1);
        
        block_offset = block_offset & mask;

    } else { // at last layer; fetch from main memory instead
        fprintf(CACHE_LOG, "RAM\n");
        // when fetching from main memory, the block offset will just be 0, since we've already requested our specific size.
        block = find_block(mem, addr.full_addr, cache->block_size);
    }

    // Fetched a block into the cache
    fprintf(CACHE_LOG, "F %d %d %d\n", cache->layer+1, addr.set_index, line_index);

    // update line
    change_validity(cache, addr.set_index, line_index, 1);
    victim->tag = addr.tag;
    victim->LRU = 0;
    if (received_cache_line_from_child != NULL) {
        change_dirtiness(cache, addr.set_index, line_index, received_cache_line_from_child->dirty);
        Address_t addr_in_child = get_address(cache->child_cache, addr.full_addr); 
        change_dirtiness(cache->child_cache, addr_in_child.set_index, get_line_index_from_tag(cache->child_cache, addr_in_child), 0);
        //received_cache_line_from_child->dirty = false; // make sure only the 'top level' version of this line is dirty
    }
    // copy and insert block
    memcpy(victim->block, block, cache->block_size);
}

// NOTE: This function assumes that cache inclusivity holds
void cache_writeback_block(Cache_t* cache, int addr_int, char* data, size_t block_size) {

    Address_t addr = get_address(cache, addr_int);

    // If inclusivity holds, we will always find a line in this set with a matching tag
    int line_index = get_line_index_from_tag(cache, addr);

    if (line_index == -1) {
        printf("ERROR: Tried to perform write-back, but block was missing in lower level cache. Cache inclusivity didn't hold.\n");
        printf("We were trying to insert tag %x into L%d\n", addr.tag, cache->layer+1);
        exit(1);
    }
    fprintf(CACHE_LOG, "E %d %d %d\n", cache->layer+1, addr.set_index, line_index);

    char* block = cache->sets[addr.set_index][line_index].block;

    // TODO: Modify this such that we index in with the proper blockoffset in case the block sizes don't match
    memcpy(block, data, block_size);

    // we only write back if the block is dirty, so the we also have to mark this line dirty
    change_dirtiness(cache, addr.set_index, line_index, 1);
    //cache->sets[addr.set_index][line_index].dirty = true;
}

int get_line_index_from_tag(Cache_t* cache, Address_t addr) {
    for (uint32_t i = 0; i < cache->associativity; i++) {
        if (cache->sets[addr.set_index][i].valid && cache->sets[addr.set_index][i].tag == addr.tag) {return i;}
    }
    return -1;
}

int get_replacement_line_index(Cache_t* cache, uint32_t set_index) {
    CacheLine_t* set = cache->sets[set_index];
    // first check if there's room anywhere.
    int line_index = -1;
    for (uint32_t i = 0; i < cache->associativity; i++) {
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
                for (uint32_t i = 0; i < cache->associativity; i++) {
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
        printf("ERROR: Couldn't find line in L%d to mark as victim.\n", cache->layer+1);
        exit(1);
    }

    return line_index;
}

void evict_cache_line(Cache_t* cache, uint32_t addr_int, CacheLine_t* evict_line, struct memory *mem) {
    Address_t addr = get_address(cache, addr_int);
    change_dirtiness(cache, addr.set_index, get_line_index_from_tag(cache, addr), 0);
    //evict_line->dirty = false;
    if (cache->child_cache == NULL) { // Writing to main memory
        memory_write_back(mem, addr_int, evict_line->block, cache->block_size);
    } else { // Writing to cache
        cache_writeback_block(cache->child_cache, addr_int, evict_line->block, cache->block_size);
    }
}

void invalidate_line(Cache_t* cache, uint32_t addr_int, struct memory* mem) {
    Address_t addr = get_address(cache, addr_int);

    int line_index = get_line_index_from_tag(cache, addr);
    if (line_index != -1) {
        CacheLine_t* victim = &cache->sets[addr.set_index][line_index];
        if (victim->dirty) {
            //victim->dirty = false;
            change_dirtiness(cache, addr.set_index, line_index, 0);
            uint32_t evict_addr = (victim->tag << (cache->set_bit_length + cache->block_offset_bit_length)) | (addr.set_index << cache->block_offset_bit_length);
            evict_cache_line(cache, evict_addr, &cache->sets[addr.set_index][line_index], mem);
        }
        // Back-invalidation up the cache chain
        if (cache->parent_cache != NULL) {
            invalidate_line(cache->parent_cache, addr_int, mem);
        }

        change_validity(cache, addr.set_index, line_index, 0);
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

    sprintf(out, " (V:%d) (D:%d) (T:%s) (LRU:%d) |", cacheLine.valid, cacheLine.dirty, tag, cacheLine.LRU);
    
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


    memset(buf, 0, sizeof(buf));

    fgets(buf, sizeof(buf), file); // name
    if (strcmp(buf, "i\n")) { // IF NOT EQUALS
        fseek(file, (long)-strlen(buf), SEEK_CUR);
    }
    else {
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
        memset(buf, 0, sizeof(buf));

        //printf("%d, %d, %d, %d\n", p, q, k, a);
        L1i = cache_new(0, (uint32_t)(pow(2,p) * q), (uint32_t)(pow(2,k)), a);
    }

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
        memset(buf, 0, sizeof(buf));

        //printf("%d, %d, %d, %d\n", p, q, k, a);
        caches[i] = *cache_new(i, (uint32_t)(pow(2,p) * q), (uint32_t)(pow(2,k)), a);
        if (i > 0) { // linked list
            caches[i-1].child_cache = &caches[i];
            caches[i].parent_cache = &caches[i-1];
        }
    }
    if (N_CACHE_LEVELS > 1 && L1i != NULL) {
        L1i->child_cache = &caches[1]; // set L2 as child of instruction cache L1i
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


Cache_t* cache_new(uint32_t layer, uint32_t cacheSize, uint32_t block_size, uint32_t associativity) {
    Cache_t* c = (Cache_t*)malloc(sizeof(Cache_t));
    c->layer = layer;
    c->cache_size = cacheSize;
    c->block_size = block_size;
    c->associativity = associativity;
    c->set_count = (c->cache_size / (c->associativity * c->block_size));

    c->block_offset_bit_length = log2(c->block_size);
    c->set_bit_length = log2(c->set_count);
    c->tag_bit_length = ADDR_LEN - c->block_offset_bit_length - c->set_bit_length;

    c->child_cache = NULL; // THIS HAS TO BE SET ELSEWHERE
    c->parent_cache = NULL; // THIS HAS TO BE SET ELSEWHERE
   
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

void start_cache_log() {
    CACHE_LOG = fopen("cache_log", "w");
}

// Returns amount of cycles spent on memory accesses
void stop_cache_log() {
    fclose(CACHE_LOG);
}

FILE* get_cache_log() {return CACHE_LOG;}

void print_all_caches() {
    printf("\n\n");
    for (uint32_t i = 0; i < N_CACHE_LEVELS; i++) {
        printf("L%d:\n", i+1);
        PrintCache(&caches[i]);
        printf("\n");
    }
}

void change_validity(Cache_t* cache, int set_index, int line_index, bool new_validity) {
    if (new_validity == 1) {
        fprintf(CACHE_LOG, "V %d %d %d\n", cache->layer+1, set_index, line_index);
    } else {
        fprintf(CACHE_LOG, "IV %d %d %d\n", cache->layer+1, set_index, line_index);
    }
    cache->sets[set_index][line_index].valid = new_validity;
    uint32_t operation_int = new_validity ? 1 : 2;
    add_operation_to_checksum(operation_int, cache->layer, set_index, line_index);
}

void change_dirtiness(Cache_t* cache, int set_index, int line_index, bool new_dirty) {
    if (new_dirty == 1) {
        fprintf(CACHE_LOG, "D %d %d %d\n", cache->layer+1, set_index, line_index);
    } else {
        fprintf(CACHE_LOG, "C %d %d %d\n", cache->layer+1, set_index, line_index);
    }
    cache->sets[set_index][line_index].dirty = new_dirty;
    uint32_t operation_int = new_dirty ? 3 : 4;
    add_operation_to_checksum(operation_int, cache->layer, set_index, line_index);
}


void add_operation_to_checksum(uint32_t type, uint32_t cache_idx, uint32_t set_idx, uint32_t line_idx) {
    cache_checksum += type * 10 + cache_idx * 100 + set_idx * 1000 + line_idx * 10000;
}

uint32_t get_cache_checksum() {return cache_checksum;}
void set_cache_checksum(uint32_t v) {cache_checksum = v;}