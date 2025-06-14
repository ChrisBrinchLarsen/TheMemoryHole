#include "lib.h"

unsigned int fib(unsigned int arg) {
  if (arg < 2) return arg;
  return fib(arg - 1) + fib(arg - 2);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    print_string("fib() missing arguments\n");
    terminate(1);
  }
  char buffer[20];
  unsigned int arg;
  unsigned int res;
  for (int i = 1; i < argc; i++) {
    arg = str_to_uns(argv[i]);
    print_string("fib(");
    print_string(argv[i]);
    print_string(") = ");
    res = fib(arg);
    uns_to_str(buffer, res);
    print_string(buffer);
    print_string("\n");
  }
  return 0;
}