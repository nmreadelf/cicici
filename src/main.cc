#include <iostream>
#include <format>

#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << argv[0] << ": invalid number of arguments" << std::endl;
    return 1;
  }

  std::cout << "  .global main" << std::endl;
  std::cout << "main:" << std::endl;
  std::cout << std::format("  mov ${}, %rax\n", std::atoi(argv[1]));
  std::cout << "  ret" << std::endl;

  return 0;
}
