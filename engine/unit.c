#include "cache.h"
#include "memory.h"
#include <stdio.h>
#include "mmu.h"
#include <stdio.h>

struct memory* memory;

#if 1 // test headers
int backInvalidationTest();
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
    memory = memory_create();
}
void test_cleanup() {
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
    int test_count = 1;
    UnitTest_t *tests = malloc(test_count * sizeof(UnitTest_t));

    tests[0] = test_new("Back Invalidation", &backInvalidationTest);

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

    initialize_cache();
    
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

    finalize_cache();
    // return true;
    return pass;
}