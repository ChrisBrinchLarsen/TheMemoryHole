#include "memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

struct memory {
  char *pages[0x10000];
};

struct memory *memory_create() {
  // When we create our memory, we malloc memory for 16 bits worth of null pointers that could later point to each page start
  return calloc(sizeof(struct memory), 1);
}

void memory_delete(struct memory *mem) {
  for (int j = 0; j < 0x10000; ++j)
  {
    if (mem->pages[j])
      free(mem->pages[j]);
  }
  free(mem);
}

char *get_page(struct memory *mem, int addr) {
  int page_number = (addr >> 16) & 0x0ffff;
  if (mem->pages[page_number] == NULL) {
    mem->pages[page_number] = calloc(65536, 1);
  }
  return mem->pages[page_number];
}

char* find_block(struct memory *mem, int addr, uint32_t block_size) {
  // TODO: This definitely needs some testing
  char* page = get_page(mem, addr);
  int block_offset_bit_length = (int)pow(2, log2(block_size));
  addr = (addr >> block_offset_bit_length) << block_offset_bit_length; // Masking out block offset bits
  int page_offset = addr & 0x0ffff;
  return &page[page_offset];
}

