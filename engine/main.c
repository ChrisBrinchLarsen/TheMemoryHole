#include "setup.h"
#include "memory.h"
#include "cache.h"
#include "mmu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void terminate(const char *error)
{
  printf("%s\n", error);
  printf("RISC-V Simulator v0.9.0: Usage:\n");
  printf("  sim riscv-dis sim-options -- prog-args\n");
  printf("    sim-options: options to the simulator\n");
  printf("      sim riscv-dis -l log     // log each instruction\n");
  //printf("      sim riscv-dis -s log     // log only summary\n");
  printf("    prog-args: arguments to the simulated program\n");
  printf("               these arguments are provided through argv. Puts '--' in argv[0]\n");
  printf("      sim riscv-dis -- gylletank   // run riscv-dis with 'gylletank' in architecture_path\n");
  exit(-1);
}

int main(int argc, char *argv[])
{
  char* arch = argv[1];
  char* dis = argv[2];

  int seperator_position = 1; // skip first, it is the path to the simulator
  int seperator_found = 0;
  while (seperator_position < argc) {
    seperator_found = strcmp(argv[seperator_position],"--") == 0;
    if (seperator_found) break;
    seperator_position++;
  }
  int num_args = argc - seperator_position;

  // if (!(argc == 3 || argc == 5)) {
  //   terminate("Missing operands");
  // }

  FILE *log_file = NULL;
  if (seperator_position == 5 && !strcmp(argv[3], "-l")) {
    log_file = fopen(argv[4], "w");
    if (log_file == NULL)
    {
      terminate("Could not open logfile, terminating.");
    }
  }
  // if (argc == 5 && !strcmp(argv[3], "-s"))
  // {
  //   log_file = fopen(argv[4], "w");
  //   if (log_file == NULL)
  //   {
  //     terminate("Could not open logfile, terminating.");
  //   }
  // }

  
  srand(time(NULL));
  open_accesses_file();
  struct memory *mem = memory_create();
  start_cache_log();
  

  if (!seperator_found) { // this case is just to avoid a illegal pointer.
    seperator_position = 0;
  }
  int retval = setup(arch, dis, &argv[seperator_position], num_args, mem, log_file);

  stop_cache_log();
  memory_delete(mem);
  close_accesses_file();
  return retval;
}
