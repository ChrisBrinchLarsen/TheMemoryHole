#include "cache.h"
#include "memory.h"
#include <stdio.h>
#include "mmu.h"
#include <stdio.h>

struct memory* memory;

#if 1 // test headers
int back_invalidation();
int back_invalidation_chain();
int ram_writeback();
int cache_writeback();
int lru_replacement();
int fifo_replacement();
#endif

#if 1 // struct and other helpers
typedef struct UnitTest {
    char* name;
    int (*fptr)();
} UnitTest_t;

UnitTest_t test_new(char* name, int (*fptr)()) {
    UnitTest_t test;
    test.name = name;
    test.fptr = fptr;
    return test;
}

char* create_cache_file(char* contents) {
    char* filename = "./unittests/cachefile.md";
    remove(filename);

    FILE *fptr;
    fptr = fopen(filename, "w");
    fprintf(fptr, "%s", contents);
    fclose(fptr);
    return filename;
}
void test_setup() {
    start_cache_log();
    memory = memory_create();
}
void test_cleanup() {
    stop_cache_log();
    // clean up caches
    if (get_l1i() != NULL) {
        free(get_l1i());
    }
    if (get_caches() != NULL) {
        free(get_caches());
    }
    // clean up memory
    memory_delete(memory);
}

#endif

int main() {
    int test_count = 6;
    UnitTest_t *tests = malloc(test_count * sizeof(UnitTest_t));
    tests[0] = test_new("Back Invalidation", &back_invalidation);
    tests[1] = test_new("Inclusivity", &back_invalidation_chain);
    tests[2] = test_new("Cache Writeback", &ram_writeback);
    tests[3] = test_new("Ram Writeback", &ram_writeback);
    tests[4] = test_new("LRU Replacement", &lru_replacement);
    tests[5] = test_new("FIFO Replacement", &fifo_replacement);

    printf("Running %d unit tests:\n", test_count);
    
    int pass_count = 0;
    for (int i = 0; i < test_count; i++) {
        printf("(%d/%d) testing '%s': ", i+1, test_count, tests[i].name);
        fflush(stdout);

        // the actual testing:
        test_setup();
        int res = tests[i].fptr();
        test_cleanup();

        if (res) { // if pass
            printf("pass\n");
            fflush(stdout);
            pass_count++;
        }
        else { // if fail
            printf("fail\n");
            fflush(stdout);
        }
    }
     if (pass_count == test_count) {
        printf("all unit tests passed.\n");
        fflush(stdout);
     }
     else {
        printf("%d tests failed.\n", test_count - pass_count);
        fflush(stdout);
     }

    free(tests);
    return 0;
}

int back_invalidation()
{
    int pass = 1;
    
    parse_cpu(create_cache_file("2\n0\nL1\n5\n1\n4\n2\nL2\n6\n1\n4\n4"));

    mmu_rd_w(memory, 0x10); // A
    mmu_rd_w(memory, 0x20); // B

    if (!get_caches()[0].sets[0][0].valid) { // make sure it was valid at some point
        pass = 0;
    }

    mmu_rd_w(memory, 0x10); // A
    mmu_rd_w(memory, 0x30); // C

    mmu_rd_w(memory, 0x10); // A
    mmu_rd_w(memory, 0x40); // D

    mmu_rd_w(memory, 0x10); // A
    mmu_rd_w(memory, 0x50); // E, this should back-invalidate A

    if (get_caches()[0].sets[0][0].valid) { // we should have back-invalidated now
        pass = 0;
    }

    // return true;
    return pass;
}

// rare case where something in L2 
int back_invalidation_chain() { // also known as inclusivity. 
    srand(5);
    //parse_cpu(create_cache_file("2\nL1\n7\n1\n4\n2\nL2\n9\n1\n4\n4\nL3\n11\n1\n4\n8"));
    parse_cpu(create_cache_file("3\n1\nL1\n5\n1\n4\n2\nL2\n6\n1\n4\n4\nL3\n6\n1\n4\n4"));
    
    //print_all_caches();
    mmu_wr_w(memory, 0x1c, 16773088);
    //print_all_caches();
    mmu_wr_w(memory, 0x20, 1);
    //print_all_caches();
    mmu_wr_w(memory, 0x30, 1);
    //print_all_caches();
    mmu_rd_instr(memory, 0x40);
    //print_all_caches();
    mmu_wr_w(memory, 0x1c, 16773088);
    //print_all_caches();

    mmu_wr_w(memory, 0x50, 1);
    //print_all_caches();

    return 1;
}

// the cache can only hold 1 word, so for it to retrieve the data it has to write to L2 correctly
int cache_writeback()
{
    parse_cpu(create_cache_file("2\n0\nL1\n2\n1\n2\n1\nL2\n3\n1\n2\n1"));

    int data = 69;

    mmu_wr_w(memory, 0x10, data); // A
    mmu_rd_w(memory, 0x20); // B
    int out = mmu_rd_w(memory, 0x10);

    return out = data;
}

// the cache can only hold 1 word, so for it to retrieve the data it has to write to ram correctly
int ram_writeback()
{
    parse_cpu(create_cache_file("1\n0\nL1\n2\n1\n2\n1"));

    int data = 69;

    mmu_wr_w(memory, 0x10, data); // A
    mmu_rd_w(memory, 0x20); // B
    int out = mmu_rd_w(memory, 0x10);
    
    return out = data;
}

// check if replacement happens in the correct order
int lru_replacement()
{
    bool pass = 1;

    parse_cpu(create_cache_file("1\n0\nL1\n6\n1\n4\n4"));

    // fill cache
    mmu_rd_w(memory, 0x10); // A
    mmu_rd_w(memory, 0x20); // B
    mmu_rd_w(memory, 0x30); // C
    mmu_rd_w(memory, 0x40); // D

    // lru: 3 2 1 0
    if (get_caches()[0].sets[0][0].LRU != 3 ||
        get_caches()[0].sets[0][1].LRU != 2 ||
        get_caches()[0].sets[0][2].LRU != 1 ||
        get_caches()[0].sets[0][3].LRU != 0) {
        pass = 0;        
    }

    // replacement
    mmu_rd_w(memory, 0x50); // D
    
    // see if block ended up in first position
    if (get_caches()[0].sets[0][0].tag != 0x5)
        pass = 0;

    // lru: 0 3 2 1
    if (get_caches()[0].sets[0][0].LRU != 0 ||
        get_caches()[0].sets[0][1].LRU != 3 ||
        get_caches()[0].sets[0][2].LRU != 2 ||
        get_caches()[0].sets[0][3].LRU != 1) {
        pass = 0;        
    }
    
    mmu_rd_w(memory, 0x30); // C again

    // lru: 1 4 0 2 // we should probably change how LRU is handled
    if (get_caches()[0].sets[0][0].LRU != 1 ||
        get_caches()[0].sets[0][1].LRU != 4 ||
        get_caches()[0].sets[0][2].LRU != 0 ||
        get_caches()[0].sets[0][3].LRU != 2) {
        pass = 0;        
    }

    return pass;
}

int fifo_replacement()
{
    bool pass = 1;

    parse_cpu(create_cache_file("1\n3\nL1\n6\n1\n4\n4"));

    // fill cache
    mmu_rd_w(memory, 0x10); // A
    mmu_rd_w(memory, 0x20); // B
    mmu_rd_w(memory, 0x30); // C
    mmu_rd_w(memory, 0x40); // D

    // fifo: 1 2 3 4
    if (get_caches()[0].sets[0][0].legacy != 1 ||
        get_caches()[0].sets[0][1].legacy != 2 ||
        get_caches()[0].sets[0][2].legacy != 3 ||
        get_caches()[0].sets[0][3].legacy != 4) {
        pass = 0;        
    }

    // replacement
    mmu_rd_w(memory, 0x50); // D
    
    // see if block ended up in first position
    if (get_caches()[0].sets[0][0].tag != 0x5)
        pass = 0;

    // fifo: 5 2 3 4
    if (get_caches()[0].sets[0][0].legacy != 5 ||
        get_caches()[0].sets[0][1].legacy != 2 ||
        get_caches()[0].sets[0][2].legacy != 3 ||
        get_caches()[0].sets[0][3].legacy != 4) {
        pass = 0;        
    }
    
    mmu_rd_w(memory, 0x30); // C again. Nothing should change

    // fifo: 5 2 3 4
    if (get_caches()[0].sets[0][0].legacy != 5 ||
        get_caches()[0].sets[0][1].legacy != 2 ||
        get_caches()[0].sets[0][2].legacy != 3 ||
        get_caches()[0].sets[0][3].legacy != 4) {
        pass = 0;        
    }

    return pass;
}