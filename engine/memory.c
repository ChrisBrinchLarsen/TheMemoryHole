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
  printf("Main memory fetching a page for address %x\n", addr);
  int page_number = (addr >> 16) & 0x0ffff;
  if (mem->pages[page_number] == NULL) {
    mem->pages[page_number] = calloc(65536, 1);
  }
  return mem->pages[page_number];
  printf("Main memory found the corresponding page at page nr %d", page_number);
}

char* find_block(struct memory *mem, int addr, uint32_t block_size) {
  printf("Main memory recieved request for finding block for address %x\n", addr);
  char* page = get_page(mem, addr);
  int block_offset_bit_length = (int)log2(block_size);
  addr = (addr >> block_offset_bit_length) << block_offset_bit_length; // Masking out block offset bits
  int page_offset = addr & 0x0ffff;
  printf("Main memory found block at page offset %d and is sending it back to requesting cache\n", page_offset);
  return &page[page_offset];
}

void memory_write_back(struct memory* mem, int addr, char* block, uint32_t block_size) {
  char* page = get_page(mem, addr);
  int page_offset = addr & 0x0ffff;
  memcpy(&page[page_offset], block, block_size);
}
