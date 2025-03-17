#include "cache.h"
#include "memory.h"
#include <stdio.h>
#include "mmu.h"

int main() {
    open_accesses_file();
    initialize_cache();
    
    printf("Unit testing starting...\n");
    open_accesses_file();
    struct memory* memory = memory_create();
    //Cache_t* cache = parse_cpu("./testing/Architectures/inclusivity_test.md");
    parse_cpu("./testing/Architectures/inclusivity_test2.md");
    //supply_cache(cache);
    

    print_all_caches();
    mmu_rd_w(memory, 0x10160);
    print_all_caches();
    mmu_rd_w(memory, 0x10164);
    print_all_caches();
    mmu_rd_w(memory, 0x1028c);
    print_all_caches();
    mmu_rd_w(memory, 0x100a0);
    print_all_caches();
    

    //mmu_wr_b(memory, 0x27, 0);
    //mmu_wr_w(memory, 0x10080, 172931);

    //uint32_t mem_cycles = finalize_cache();
    finalize_cache();
    close_accesses_file();
}