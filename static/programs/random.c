#include <stdlib.h>

int main(int argc, char* argv[]) {
  char buffer[1048576];
  for (int i = 0; i < 1000; i++) {
    buffer[rand() % 1048576] = 69;
  }

  char c = 0;
  for (int j = 0; j < 1000; j++) {
    c = buffer[rand() % 1048576];
  }

  return 0;
}

