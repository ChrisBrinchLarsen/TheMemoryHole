#include "cache.h"
#include "memory.h"

int main() {
    struct memory* memory = memory_create();
    Cache_t** caches = ParseCPUArchitecture("./Testing/Architectures/SimpleCPU.md");
    Cache_t* L1 = caches[0];
    cache_rd_b(memory, L1, 0b10000100000010000000010000001000);
    cache_rd_b(memory, L1, 0b0000100000001000001000000010000);
    cache_rd_b(memory, L1, 0b00010000000001000000100010000100);
    PrintCache(L1);
}