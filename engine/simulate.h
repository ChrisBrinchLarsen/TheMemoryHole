#ifndef __SIMULATE_H__
#define __SIMULATE_H__

#include "memory.h"
#include "assembly.h"
#include "hashmap/hashmap.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct ProgramLineMap {
    int pc;
    int start;
    int end;
} ProgramLineMap_t;
int programLineMap_compare(const void *a, const void *b, void *udata);
bool programLineMap_iter(const void *item, void *udata);
uint64_t programLineMap_hash(const void *item, uint64_t seed0, uint64_t seed1);

// Simuler RISC-V program i givet lager og fra given start adresse
long int simulate(struct memory *mem, struct assembly *as, int start_addr, FILE *log_file);

#endif
