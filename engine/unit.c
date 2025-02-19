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
    printf("*(0b00101010110101101010101010110101) = %d\n", 69);
    cache_wr_b(L1, memory, 0b00101010110101101010101010110101, 69);
    printf("*(0b00101010110101101010101010110101) = %d\n", cache_rd_b(L1, memory, 0b00101010110101101010101010110101));
    
    //cache_rd_b(L1, memory, 0b0000100000001000001000000010000);
    //cache_rd_b(L1, memory, 0b00010000000001000000100010000100);
    //PrintCache(L1);
    //PrintCache(L1->childCache);
}