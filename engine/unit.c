#include "cache.h"
#include "memory.h"
#include <stdio.h>
#include "mmu.h"
#include <stdio.h>

struct memory* memory;

#if 1 // test headers
int backInvalidationTest();
int inclusivity_test();
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
    int test_count = 2;
    UnitTest_t *tests = malloc(test_count * sizeof(UnitTest_t));

    tests[0] = test_new("Back Invalidation", &backInvalidationTest);
    tests[1] = test_new("Inclusivity", &inclusivity_test);

    printf("Running %d unit tests:\n", test_count);
    
    int pass_count = 0;
    for (int i = 0; i < test_count; i++) {
        printf("(%d/%d) testing '%s': ", i+1, test_count, tests[i].name);

        // the actual testing:
        test_setup();
        int res = tests[i].fptr();
        test_cleanup();

        if (res) { // if pass
            printf("pass\n");
            pass_count++;
        }
        else { // if fail
            printf("fail\n");
        }
    }
     if (pass_count == test_count) {
        printf("all unit tests passed.\n");
     }
     else {
        printf("%d tests failed.\n", test_count - pass_count);
     }

    free(tests);
    return 0;
}

int backInvalidationTest()
{
    int pass = 1;
    
    parse_cpu(create_cache_file("2\nL1\n5\n1\n4\n2\nL2\n6\n1\n4\n4"));


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

int inclusivity_test() {
    //parse_cpu(create_cache_file("2\nL1\n7\n1\n4\n2\nL2\n9\n1\n4\n4\nL3\n11\n1\n4\n8"));
    parse_cpu(create_cache_file("3\nL1\n5\n1\n4\n2\nL2\n6\n1\n4\n4\nL3\n6\n1\n4\n4"));
    
    printf("\n");

    print_all_caches();
    mmu_wr_w(memory, 0x1c, 16773088);
    print_all_caches();
    mmu_wr_w(memory, 0x20, 1);
    print_all_caches();
    mmu_wr_w(memory, 0x30, 1);
    print_all_caches();
    mmu_rd_instr(memory, 0x40);
    print_all_caches();
    mmu_wr_w(memory, 0x1c, 16773088);
    print_all_caches();

    mmu_wr_w(memory, 0x50, 1);
    print_all_caches();

    return 1;
}