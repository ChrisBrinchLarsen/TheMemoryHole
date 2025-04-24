#ifndef __CACHE_H__
#define __CACHE_H__

#include "memory.h"
#include <stdbool.h> 
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct Address {
    uint32_t full_addr;
    uint32_t tag;
    uint32_t set_index;
    uint32_t block_offset;
} Address_t;

typedef struct CacheLine {
    bool valid;
    bool dirty;
    uint32_t tag;
    uint32_t LRU;
    char* block;
} CacheLine_t;


typedef struct Cache {
    uint32_t layer;

    // base parameters
    uint32_t cache_size;
    uint32_t associativity;

    uint32_t set_count;

    // bit sizes
    uint32_t block_offset_bit_length;
    uint32_t set_bit_length;
    uint32_t tag_bit_length;

    uint32_t block_size;

    struct Cache* child_cache; // Cache_t name doesn't exist here
    struct Cache* parent_cache; // Cache_t name doesn't exist here

    CacheLine_t **sets;
} Cache_t;


Cache_t* get_caches();
Cache_t* get_l1i();

/**
 * Parses a cache architecture file to generate cache hierarchy data structure
 * 
 * @param path A string containing the path to a CPU architecture specification file
 * @return An array of caches, with higher level caches appearing first
 */
Cache_t* parse_cpu(char* path);

/**
 * Writes a single word (4 bytes) to memory
 * 
 * @param mem A pointer to the targeted backing memory struct 
 * @param addr The address to write data to
 * @param data The data to write to memory 
 */
void cache_wr_w(struct memory *mem, int addr, uint32_t data);

/**
 * Writes a half (2 bytes) to memory
 * 
 * @param mem A pointer to the targeted backing memory struct 
 * @param addr The address to write data to
 * @param data The data to write to memory
 */
void cache_wr_h(struct memory *mem, int addr, uint16_t data);

/**
 * Writes a byte to memory
 * 
 * @param mem A pointer to the targeted backing memory struct 
 * @param addr The address to write data to
 * @param data The data to write to memory
 */
void cache_wr_b(struct memory *mem, int addr, uint8_t data);

int cache_rd_instr(struct memory *mem, int addr_int);

/**
 * Reads a single word (4 bytes) from memory
 * 
 * @param mem A pointer to the targeted backing memory struct 
 * @param addr The address to read from
 * @return The word retrieved from memory at addr as an integer
 */
int cache_rd_w(struct memory *mem, int addr);

/**
 * Reads a half (2 bytes) from memory
 * 
 * @param mem A pointer to the targeted backing memory struct 
 * @param addr The address to read from
 * @return The half retrieved from memory at addr, zero extended as an integer
 */
int cache_rd_h(struct memory *mem, int addr);

/**
 * Reads a byte from memory
 * 
 * @param mem A pointer to the targeted backing memory struct 
 * @param addr The address to read from
 * @return The byte retrieved from memory at addr, zero extended as an integer
 */
int cache_rd_b(struct memory *mem, int addr);


/**
 * Initalizes the active cache
 * 
 * @note Really just opens a cache_log file in the scope of the cache.c file
 */
void initialize_cache();

/**
 * Finalizes the active cache
 * 
 * @note Really just closes a cache_log file in the scope of the cache.c file
 */
void finalize_cache();

/**
 * Retrieves the active cache_log file pointer
 * 
 * @return A file pointer to an opened cache_log file
 */
FILE* get_cache_log();

/**
 * Retrieves the active cache_log file pointer
 * 
 */
void print_all_caches();

#endif