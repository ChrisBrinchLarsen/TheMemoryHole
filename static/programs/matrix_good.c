#include "lib.h"

#define SIZE 25

int main(int argc, char* argv[]) {
  char buffer[20];
  int matrix[SIZE][SIZE];
  
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      matrix[i][j] = 1;
    }
  }

  unsigned int sum = 0;
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      sum += matrix[i][j];
    }
  }

  uns_to_str(buffer, sum);
  print_string(buffer);
  print_string("\n");
  return 0;
}

