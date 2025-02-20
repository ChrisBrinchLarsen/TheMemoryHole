#include "cache.h"
#include "memory.h"
#include <stdio.h>
#include "mmu.h"

int main() {
    printf("Unit testing starting...\n");
    struct memory* memory = memory_create();
    printf("Memory successfully created\n");
    Cache_t** caches = ParseCPUArchitecture("./testing/Architectures/SimpleCPU.md");
    printf("CPU Architecture Parsed succesfully\n");
    Cache_t* L1 = caches[0];
    supply_cache(L1);

    memory_wr_b(memory, 0xf, 0);
    memory_wr_b(memory, 0x27, 0);
    memory_wr_w(memory, 0x10080, 172931);
}