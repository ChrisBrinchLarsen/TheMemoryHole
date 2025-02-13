#include "memory.h"
#include <stdlib.h>
#include <stdio.h>

struct memory
{
  int *pages[0x10000];
};

struct memory *memory_create()
{
  return calloc(sizeof(struct memory), 1);
}

void memory_delete(struct memory *mem)
{
  for (int j = 0; j < 0x10000; ++j)
  {
    if (mem->pages[j])
      free(mem->pages[j]);
  }
  free(mem);
}

int *get_page(struct memory *mem, int addr)
{
  int page_number = (addr >> 16) & 0x0ffff;
  if (mem->pages[page_number] == NULL)
  {
    mem->pages[page_number] = calloc(65536, 1);
  }
  return mem->pages[page_number];
}

void memory_wr_w(struct memory *mem, int addr, uint32_t data) {
}

void memory_wr_h(struct memory *mem, int addr, uint16_t data) {
}

void memory_wr_b(struct memory *mem, int addr, uint8_t data) {
}

int memory_rd_w(struct memory *mem, int addr) {
}

int memory_rd_h(struct memory *mem, int addr) {
}

int memory_rd_b(struct memory *mem, int addr) {
}
