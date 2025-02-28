#include "cache.h"
#include "memory.h"
#include <stdio.h>
#include "mmu.h"

int main() {
    printf("Unit testing starting...\n");
    struct memory* memory = memory_create();
    initialize_cache();
    Cache_t** caches = ParseCPUArchitecture("./testing/Architectures/SimpleCPU.md");
    supply_cache(caches[0]);

    memory_wr_b(memory, 0xf, 0);
    memory_wr_b(memory, 0x27, 0);
    memory_wr_w(memory, 0x10080, 172931);

    uint32_t mem_cycles = finalize_cache();
    int N_CACHE_LAYERS = get_cache_layer_count();
}