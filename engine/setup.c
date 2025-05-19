#include "memory.h"
#include "assembly.h"
#include "read_exec.h"
#include "simulate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <regex.h>
#include "cache.h"
#include "mmu.h"
#include "hashmap/hashmap.h"



void pass_args_to_program(struct memory* mem, char* argv[], int num_args) {
  if (num_args == 0) {
    return;
  }
  // insert args into main memory
  unsigned count_addr = 0x1000000;
  unsigned argv_addr = 0x1000004;
  unsigned str_addr = argv_addr + 4 * num_args;
  mmu_wr_w_instr(mem, count_addr, num_args);
  for (int index = 0; index < num_args; ++index) {
    mmu_wr_w_instr(mem, argv_addr + 4 * index, str_addr);
    char* cp = argv[index];
    int c;
    do {
      c = *cp++;
      mmu_wr_b_instr(mem, str_addr++, c);
    } while (c);
  }
}

struct hashmap* createInstructionHashmap(const char *filename) {
  struct hashmap *map = hashmap_new(sizeof(ProgramLineMap_t), 0, 0, 0, programLineMap_hash, programLineMap_compare, NULL, NULL);

  FILE *fp;
  fp = fopen(filename, "r");
  char buffer[256];

  int numbuffsize = 8;
  char numbuffer[8];

  regex_t lineRegex;
  //char re[] = "\\/\\/\\[\\[(\\\d+)\\]\\]";
  char linere[] = "\\[\\[([0-9]+)\\]\\]";
  regcomp(&lineRegex, linere, REG_EXTENDED); //matches //[[(\d+)]] and gives \d+ out
  
  regex_t pcRegex;
  //char pcre[] = "([[:digit:]]+):";
  char pcre[] = "([a-f0-9]+):";
  regcomp(&pcRegex, pcre, REG_EXTENDED);


  regmatch_t pmatches[2];

  int startLine;
  int endLine;
  int pc;

  while (fgets(buffer, sizeof(buffer), fp) != NULL) { 

    // find a line number
    if (regexec(&lineRegex, buffer, 2, pmatches, 0) == 0) {
      memset(numbuffer, 0, numbuffsize);
      memcpy(numbuffer, buffer + pmatches[1].rm_so, pmatches[1].rm_eo - pmatches[1].rm_so);
      startLine = atoi(numbuffer); 
      endLine = startLine;

      // keep going until we find the next RISC-V line
      while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // once we find a RISC-V line, we save the address / program counter:
        if (regexec(&pcRegex, buffer, 2, pmatches, 0) == 0) {
          memset(numbuffer, 0, numbuffsize);
          memcpy(numbuffer, buffer + pmatches[1].rm_so, pmatches[1].rm_eo - pmatches[1].rm_so);
          pc = (int)strtol(numbuffer, NULL, 16);
          break;
        }
        // sometimes we might have multiple C lines in a row, in which case we update endline instead
        if (regexec(&lineRegex, buffer, 2, pmatches, 0) == 0) {
          memset(numbuffer, 0, numbuffsize);
          memcpy(numbuffer, buffer + pmatches[1].rm_so, pmatches[1].rm_eo - pmatches[1].rm_so);
          endLine = atoi(numbuffer);
          continue;
        }
      }
      hashmap_set(map, &(ProgramLineMap_t){.pc=pc,.start=startLine,.end=endLine});
      // back to finding start lines again
    }
  }

  fclose(fp);

  return map;
}

int setup(char* architecture_path, char* dis_path, char* program_args[], int num_args, struct memory* mem, FILE* log_file)
{
  struct hashmap *map = createInstructionHashmap(dis_path);

  parse_cpu(architecture_path);
  pass_args_to_program(mem, program_args, num_args);

  struct assembly *as = assembly_create();

  int start_addr = read_exec(mem, as, dis_path, log_file);
  clock_t before = clock();
  long int num_insns = simulate(mem, as, start_addr, log_file, map);
  clock_t after = clock();

  int ticks = after - before;
  double mips = (1.0 * num_insns * CLOCKS_PER_SEC) / ticks / 1000000;

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

  return 0;
}
