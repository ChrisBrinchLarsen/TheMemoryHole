#include "cache.h"
#include "memory.h"
#include <stdio.h>

int main() {
    printf("Unit testing starting...\n");
    struct memory* memory = memory_create();
    printf("Memory successfully created\n");
    Cache_t** caches = ParseCPUArchitecture("./testing/Architectures/SimpleCPU.md");
    printf("CPU Architecture Parsed succesfully\n");
    Cache_t* L1 = caches[0];
    cache_rd_b(L1, memory, 0b10000100000010000000010000001000);
    //cache_rd_b(L1, memory, 0b0000100000001000001000000010000);
    //cache_rd_b(L1, memory, 0b00010000000001000000100010000100);
    PrintCache(L1);
    PrintCache(L1->childCache);
}