#include "cache.h"
#include "memory.h"
#include <stdio.h>
#include "mmu.h"

int main() {
    printf("Unit testing starting...\n");
    open_accesses_file();
    struct memory* memory = memory_create();
    initialize_cache();
    parse_cpu("./testing/Architectures/SimpleCPU.md");

    print_all_caches();
    mmu_wr_w(memory, 0x11538, 0);
    print_all_caches();
    mmu_rd_w(memory, 0x103f0);
    print_all_caches();
    

    finalize_cache();
}