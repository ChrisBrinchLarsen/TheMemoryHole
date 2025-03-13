#include "cache.h"
#include "memory.h"
#include <stdio.h>
#include "mmu.h"

int main() {
    open_accesses_file();
    initialize_cache();
    
    printf("Unit testing starting...\n");
    struct memory* memory = memory_create();
    Cache_t** caches = ParseCPUArchitecture("./testing/Architectures/inclusivity_test.md");
    supply_cache(caches[0]);
    
    PrintCache(caches[0]);
    PrintCache(caches[1]);

    mmu_wr_b(memory, 0b00100, 0); // A
    mmu_wr_b(memory, 0b01000, 0); // B

    mmu_wr_b(memory, 0b00100, 0); // A
    mmu_wr_b(memory, 0b01100, 0); // C
    
    mmu_wr_b(memory, 0b00100, 0); // A
    mmu_wr_b(memory, 0b10000, 0); // D
    
    mmu_wr_b(memory, 0b00100, 0); // A

    PrintCache(caches[0]);
    PrintCache(caches[1]);

    mmu_wr_b(memory, 0b10100, 0); // E

    PrintCache(caches[0]);
    PrintCache(caches[1]);

    mmu_wr_b(memory, 0b01000, 0); // E
    
    PrintCache(caches[0]);
    PrintCache(caches[1]);

    //mmu_wr_b(memory, 0x27, 0);
    //mmu_wr_w(memory, 0x10080, 172931);

    uint32_t mem_cycles = finalize_cache();
    close_accesses_file();
}