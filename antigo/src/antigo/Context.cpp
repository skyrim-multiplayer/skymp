#include "antigo/Context.h"

#include <cassert>
#include <exception>
#include <memory>

#include <cpptrace/cpptrace.hpp>

namespace Antigo {

// XXX probably should be moved
struct ExecutionData
{
  std::vector<const ContextImpl*> contextChain;
  // std::vector<std::unique_ptr<ContextImpl>> exceptionWitnesses;
  std::vector<ContextImpl> exceptionWitnesses;
};

class ContextImpl
{
public:
  ContextImpl(const char* filename_, size_t linenum_, const char* funcname_)
    : filename(filename_), linenum(linenum_), funcname(funcname_), rawTrace(cpptrace::generate_raw_trace()), uncaughtExceptions(std::uncaught_exceptions()), prev(nullptr), moved(false)
    {
      // std::cerr << "=== ctor\n";
      auto& chain = GetCurrentExecutionData().contextChain;
      if (!chain.empty()) {
        prev = chain.back();
      }
      // std::cerr << "prev " << prev << "\n";
      chain.push_back(this);
      // std::cerr << "chain push " << this << "\n";
      // std::cerr << "chain chain size " << chain.size() << "\n";
    }
  
  ~ContextImpl() {
    // std::cerr << "=== dtor\n";
    if (moved) {
      // std::cerr << "moved\n";
      return;
    }
    auto& edata = GetCurrentExecutionData();
    auto& chain = edata.contextChain;
    auto& witnesses = edata.exceptionWitnesses;
    // std::cerr << "chain chain size " << chain.size() << "\n";
    // std::cerr << "chain top " << (chain.empty() ? nullptr : chain.back()) << "\n";
    assert(!chain.empty() && chain.back() == this);
    chain.pop_back();

    if (std::uncaught_exceptions() != uncaughtExceptions) {
      std::cerr << "Exception in this context's scope\n";
      std::cerr << "Macro was called at " << filename << ":" << linenum << " in " << funcname << "\n";

      // if (!witnesses.empty() && witnesses.back().prev == this) {
      //   std::cerr << "this one was overshadowed\n";
      //   return;
      // }
      prev = nullptr; // TODO: provide a fully-fledged trace here. Potentially. But probably we'd be better off just using cpptrace for that
      witnesses.push_back(std::move(*this));
      witnesses.back().moved = true;  // XXX rework to get rid of this flag (another class or something)

      // rawTrace.resolve().print();
    }
  }

  void PrintTrace() const
  {
    std::cerr << "Hello from PrintTrace()!\n";
    std::cerr << "Macro was called at " << filename << ":" << linenum << " in " << funcname << "\n";
    rawTrace.resolve().print();
  }

  // Context::Context& operator=()
private:
  const char* filename;
  size_t linenum;
  const char* funcname;
  cpptrace::raw_trace rawTrace;
  int uncaughtExceptions;
  const ContextImpl* prev;
  bool moved;
};

Context::Context(std::unique_ptr<ContextImpl>pImpl_): pImpl(std::move(pImpl_)) {}

Context::Context(const char* filename_, size_t linenum_, const char* funcname_)
: pImpl(std::make_unique<ContextImpl>(filename_, linenum_, funcname_)) {}

Context::~Context() = default;

void Context::PrintTrace() const {
  pImpl->PrintTrace();
}

// XXX probably should be moved
// XXX maybe anon ns
ExecutionData& GetCurrentExecutionData() {
  thread_local ExecutionData data;
  return data;
}

bool HasExceptionWitness() {
  return !GetCurrentExecutionData().exceptionWitnesses.empty();
}

// Context PopExceptionWitness() {
//   auto& w = GetCurrentExecutionData().exceptionWitnesses;
//   auto ctxPImpl = std::move(w.back());
//   w.pop_back();
//   return Context(std::move(ctxPImpl));
// }

Context PopExceptionWitness() {
  auto& w = GetCurrentExecutionData().exceptionWitnesses;
  auto ctx= std::move(w.back());
  w.pop_back();
  return Context(std::make_unique<ContextImpl>(std::move(ctx)));
}

} // namespace Antigo
