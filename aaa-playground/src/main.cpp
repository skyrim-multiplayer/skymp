#include <exception>
#include <iostream>
#include <stdexcept>

// #include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>

#include "antigo/Context.h"

void test3(int x)
{
  ANTIGO_CONTEXT_INIT(ctx);
  ctx.AddMessage("checking");
  // ctx.AddMessage(x);
  if (x == 7) {
    ctx.AddMessage("matched to throw");
    throw std::runtime_error("baibai");
  }
}

void test2(int start)
{
  ANTIGO_CONTEXT_INIT(ctx);
  for (int i = start; i < 10; ++i) {
    ctx.AddMessage("hi from loop"); // discouraged
    test3(i);
  }
}

void test()
{
  ANTIGO_CONTEXT_INIT(ctx);
  std::cout << "hello test\n";
  test2(3);
  std::cout << "goodbye test\n";
}

int main()
{
  // ANTIGO_CONTEXT_INIT(ctx);
  std::cout << "hello world\n";

  try {
    throw std::runtime_error("kek1");
  } catch (const std::exception& e) {
    std::cout << "caught " << e.what() << "\n";
    std::cout << "uncaught_exceptions: " << std::uncaught_exceptions() << "\n";
    try {
      throw std::runtime_error("kek1");
    } catch (const std::exception& e) {
      std::cout << "caught " << e.what() << "\n";
      std::cout << "uncaught_exceptions: " << std::uncaught_exceptions() << "\n";
    }
    std::cout << "uncaught_exceptions: " << std::uncaught_exceptions() << "\n";
    // std::cout << "stacktrace :" << "\n";
    // // cpptrace::from_current_exception().print();
    // for (size_t i = 0; Antigo::HasExceptionWitness(); ++i) {
    //   std::cerr << "Exception witness #" << i << "\n";
    //   Antigo::PopExceptionWitness().PrintTrace();
    // }
  }

  /*
  try {
    ::cpptrace ::detail ::try_canary cpptrace_try_canary;
    try {
      {
        test();
      }
    } catch (::cpptrace ::detail ::unwind_interceptor&) {
    }
  } catch (const std ::exception& e) {
    std::cout << "caught " << e.what() << "\n";
    std::cout << "stacktrace :" << "\n";
    cpptrace::from_current_exception().print();
    for (size_t i = 0; Antigo::HasExceptionWitness(); ++i) {
      std::cerr << "Exception witness #" << i << "\n";
      Antigo::PopExceptionWitness().PrintTrace();
    }
  }
  */
}
