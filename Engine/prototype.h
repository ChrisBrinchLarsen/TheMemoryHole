#include <stdbool.h> 
#include <stdint.h>

#define BLOCK_SIZE 16
typedef struct CacheLine {
    bool valid;
    uint32_t tag;
    uint32_t LRU;
    char block[BLOCK_SIZE];
} CacheLine_t;

void ReadMemory(uint32_t address);
int IsLineInSet(CacheLine_t *set, uint32_t tag);
void InsertLineInSet(CacheLine_t *set, uint32_t tag);
void UpdateCacheSet(CacheLine_t *set);
void CacheSetToString(CacheLine_t** Cache, int setIndex, char* out);
void CacheLineToString(CacheLine_t cacheLine, char* out);