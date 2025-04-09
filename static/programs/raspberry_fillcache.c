#include "lib.h"

int main(int argc, char* argv[]) {
  char buffer[20];
  int arr[131072];


  for (int i = 0; i < 8192; i+=16) {
    arr[i] = 1;
  }

  int sum = 0;
  for (int i = 0; i < 8192; i+=16) {
    sum += arr[i];
  }
  
  return sum;
}