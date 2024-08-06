#include <iostream>
#include <fmt/core.h>

#include <cstdlib>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << argv[0] << ": invalid number of arguments" << std::endl;
    return 1;
  }

  char *p = argv[1];

  fmt::print("  .global main\n");
  fmt::print("main:\n");
  fmt::print("  mov ${}, %rax\n", strtol(p, &p, 10));

  while (*p) {
      if (*p == '+') {
          p++;
          fmt::print("  add ${}, %rax\n", strtol(p, &p, 10));
          continue;
      }
      if (*p == '-') {
          p++;
          fmt::print("  sub ${}, %rax\n", strtol(p, &p, 10));
          continue;
      }

      std::cerr << "unexpected character: '" << *p << "'\n";
      return 1;
  }
  fmt::print("  ret\n");

  return 0;
}
