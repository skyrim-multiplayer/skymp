#pragma once

#include <exception>
#include <iostream>
#include <memory>

namespace Antigo {

class ContextImpl;

class Context
{
public:
  Context(std::unique_ptr<ContextImpl>pImpl_);
  Context(const char* filename_, size_t linenum_, const char* funcname_);

  ~Context();

  void PrintTrace() const;

private:
  std::unique_ptr<ContextImpl> pImpl;
  // XXX TODO fast pimpl
};

#define ANTIGO_CONTEXT_INIT(ctx) Antigo::Context ctx(__FILE__, __LINE__, __func__)

struct ExecutionData;
ExecutionData& GetCurrentExecutionData();

bool HasExceptionWitness();
Context PopExceptionWitness();

} // namespace Antigo
