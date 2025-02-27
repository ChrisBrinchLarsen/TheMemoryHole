#include "memory.h"
#include "assembly.h"
#include "read_exec.h"
#include "simulate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cache.h"
#include "mmu.h"

void terminate(const char *error)
{
  printf("%s\n", error);
  printf("RISC-V Simulator v0.9.0: Usage:\n");
  printf("  sim riscv-dis sim-options -- prog-args\n");
  printf("    sim-options: options to the simulator\n");
  printf("      sim riscv-dis -l log     // log each instruction\n");
  printf("      sim riscv-dis -s log     // log only summary\n");
  printf("    prog-args: arguments to the simulated program\n");
  printf("               these arguments are provided through argv. Puts '--' in argv[0]\n");
  printf("      sim riscv-dis -- gylletank   // run riscv-dis with 'gylletank' in argv[1]\n");
  exit(-1);
}

int pass_args_to_program(struct memory* mem, int argc, char* argv[]) {
  int seperator_position = 1; // skip first, it is the path to the simulator
  int seperator_found = 0;
  while (seperator_position < argc) {
    seperator_found = strcmp(argv[seperator_position],"--") == 0;
    if (seperator_found) break;
    seperator_position++;
  }
  if (seperator_found) { // we've got args for the program!!
    // the seperator is the first arg.
    int first_arg = seperator_position;
    int num_args = argc - first_arg;
    unsigned count_addr = 0x1000000;
    unsigned argv_addr = 0x1000004;
    unsigned str_addr = argv_addr + 4 * num_args;
    memory_wr_w(mem, count_addr, num_args);
    for (int index = 0; index < num_args; ++index) {
      memory_wr_w(mem, argv_addr + 4 * index, str_addr);
      char* cp = argv[first_arg + index];
      int c;
      do {
        c = *cp++;
        memory_wr_b(mem, str_addr++, c);
      } while (c);
    }
  }
  // leave it to main to handle args before the seperator
  return seperator_position;
}

int main(int argc, char *argv[])
{
  struct memory *mem = memory_create();
  initialize_cache();
  Cache_t** caches = ParseCPUArchitecture(argv[1]);
  supply_cache(caches[0]); // Letting MMU know which cache should be checked first
  argc = pass_args_to_program(mem, argc, argv);
  if (argc == 3 || argc == 5)
  {
    struct assembly *as = assembly_create();
    FILE *log_file = NULL;
    if (argc == 5 && !strcmp(argv[3], "-l"))
    {
      log_file = fopen(argv[4], "w");
      if (log_file == NULL)
      {
        terminate("Could not open logfile, terminating.");
      }
    }
    int start_addr = read_exec(mem, as, argv[2], log_file);
    clock_t before = clock();
    long int num_insns = simulate(mem, as, start_addr, log_file);
    clock_t after = clock();
    uint32_t mem_cycles = finalize_cache();
    int N_CACHE_LAYERS = get_cache_layer_count();
    int misses, hits, total_hits;
    for (int i = 0; i < N_CACHE_LAYERS; i++) {
      misses = get_misses_at_layer(i);
      hits = get_hits_at_layer(i);
      total_hits += hits;

      printf("L%d - %0.3f (%d/%d)",
              i+1,
              (float)hits/(hits + misses),
              hits,
              hits + misses);

      // printf("L%d - Hit: %d - Miss: %d - Hit-rate: %0.3f - Miss-rate %0.3f\n",
      //           i+1,
      //           hits,
      //           misses,
      //           (float)hits/(hits + misses),
      //           (float)misses/(hits + misses));
    }
    printf("In total handled %d memory accesses.\n", total_hits + get_misses_at_layer(N_CACHE_LAYERS-1));
    printf("Total cache hit-rate: %0.3f\n", (float)total_hits/(total_hits + get_misses_at_layer(N_CACHE_LAYERS-1)));
    printf("Clock cycles in memory: %d\n", mem_cycles);

    int ticks = after - before;
    double mips = (1.0 * num_insns * CLOCKS_PER_SEC) / ticks / 1000000;
    if (argc == 5 && !strcmp(argv[3], "-s"))
    {
      log_file = fopen(argv[4], "w");
      if (log_file == NULL)
      {
        terminate("Could not open logfile, terminating.");
      }
    }
    if (log_file)
    {
      fprintf(log_file, "\nSimulated %ld instructions in %d ticks (%f MIPS)\n", num_insns, ticks, mips);
      fclose(log_file);
    }
    else
    {
      printf("\nSimulated %ld instructions in %d ticks (%f MIPS)\n", num_insns, ticks, mips);
    }
    assembly_delete(as);
    memory_delete(mem);
  }
  else {
    terminate("Missing operands");
    memory_delete(mem);
  }
}
