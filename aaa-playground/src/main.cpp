#include <exception>
#include <iostream>
#include <stdexcept>
#include "antigo/Context.h"

void test3(int x) {
  ANTIGO_CONTEXT_INIT(ctx);
  if (x == 7) {
    throw std::runtime_error("baibai");
  }
}

void test2(int start) {
  ANTIGO_CONTEXT_INIT(ctx);
  for (int i = start; i < 10; ++i) {
    test3(i);
  }
}

void test() {
  ANTIGO_CONTEXT_INIT(ctx);
  std::cout << "hello test\n";
  test2(3);
  std::cout << "goodbye test\n";
}

int main() {
  ANTIGO_CONTEXT_INIT(ctx);
  std::cout << "hello world\n";

  try {
    test();
  } catch (const std::exception& e) {
    std::cout << "caught " << e.what() << "\n";
    for (size_t i = 0; Antigo::HasExceptionWitness(); ++i) {
      std::cerr << "Exception witness #" << i << "\n";
      Antigo::PopExceptionWitness().PrintTrace();
    }
  }
}
