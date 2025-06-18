#include <stdlib.h>
#include "lib.h"

#define SIZE 10

int main(int argc, char* argv[]) {

  int* numbers = (int*)allocate(SIZE);
  for (int i = 0; i < SIZE; i++) {
    numbers[i] = 1;
  }
  int sum = 0;
  for (int i = 0; i < SIZE; i++) {
    sum += numbers[i];
  }
  release(numbers);
  char buffer[16];
  uns_to_str(buffer, sum);
  print_string(buffer);
  return sum;
}

