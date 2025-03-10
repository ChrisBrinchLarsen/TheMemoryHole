#include "lib.h"

int main(int argc, char* argv[]) {
  char buffer[20];
  int matrix[25][25];
  
  for (int i = 0; i < 25; i++) {
    for (int j = 0; j < 25; j++) {
      matrix[i][j] = 1;
    }
  }

  unsigned int sum = 0;
  for (int i = 0; i < 25; i++) {
    for (int j = 0; j < 25; j++) {
      sum += matrix[i][j];
    }
  }

  uns_to_str(buffer, sum);
  print_string(buffer);
  print_string("\n");
  return 0;
}

