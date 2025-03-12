#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

struct memory;

// opret/nedlæg lager
struct memory *memory_create();
void memory_delete(struct memory *);

char *get_page(struct memory *mem, int addr);

char* find_block(struct memory *mem, int addr, uint32_t block_size);
void memory_write_back(struct memory* mem, int addr, char* block, uint32_t block_size);

void memory_wr_w(struct memory *mem, int addr, uint32_t data);
void memory_wr_h(struct memory *mem, int addr, uint16_t data);
void memory_wr_b(struct memory *mem, int addr, uint8_t data);

#endif
