#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

struct memory;

// opret/nedlæg lager
struct memory *memory_create();
void memory_delete(struct memory *);

char *get_page(struct memory *mem, int addr);

char* find_block(struct memory *mem, int addr, uint32_t block_size);

#endif
