#pragma once

#include <cstdint>
#include <memory>

#include "antigo/ResolvedContext.h"
#include "antigo/impl/complex_values.h"

namespace Antigo {
struct ResolvedContext;

class OnstackContext
{
public:
  OnstackContext(const char* filename_, size_t linenum_, const char* funcname_) {}
  ~OnstackContext() {}

  OnstackContext(OnstackContext&& other) = delete;
  OnstackContext(const OnstackContext& other) = delete;
  OnstackContext& operator=(OnstackContext&& other) = delete;
  OnstackContext& operator=(const OnstackContext& other) = delete;

  void AddMessage(const char* message) {}

  void AddPtr(const void* ptr) {}
  void AddUnsigned(uint64_t val) {}
  void AddSigned(int64_t val) {}

  template <class T>
  void AddPtr(const std::shared_ptr<T>& ptr) {
    AddPtr(ptr.get());
  }

  void AddLambdaWithOwned(std::function<std::string()> printerFunc) {}
  [[nodiscard]] impl::ReferencedValueGuard AddLambdaWithRef(std::function<std::string()> printerFunc) { return {}; }

  [[nodiscard]] ResolvedContext Resolve() const { return {}; }
  void Orphan() const {}

  [[deprecated("stub is being used")]]
  void LogInnerExecution() {}

  bool IsLoggingInnerExecution() const { return false; }
};

#define ANTIGO_CONTEXT_INIT(ctx) ::Antigo::OnstackContext ctx(__FILE__, __LINE__, __func__)

} // namespace Antigo
