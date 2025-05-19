#ifndef __SETUP_H__
#define __SETUP_H__

#include <stdio.h>
#include "memory.h"

int setup(char* architecture_path, char* dis_path, char* program_args[], int num_args, struct memory *mem, FILE* log_file);

#endif