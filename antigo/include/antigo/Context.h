#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "third_party/fast_pimpl.hpp"

#include "antigo/impl/complex_values.h"

namespace Antigo {

class ResolvedContext;

class OnstackContextImpl;

class OnstackContext
{
public:
  OnstackContext(const char* filename_, size_t linenum_, const char* funcname_);
  ~OnstackContext();

  OnstackContext(OnstackContext&& other) = delete;
  OnstackContext(const OnstackContext& other) = delete;
  OnstackContext& operator=(OnstackContext&& other) = delete;
  OnstackContext& operator=(const OnstackContext& other) = delete;

  // === add messages ===
  void AddMessage(const char* message);

  void AddPtr(const void* ptr);
  void AddUnsigned(uint64_t val);
  void AddSigned(int64_t val);

  template <class T>
  void AddPtr(const std::shared_ptr<T>& ptr) {
    AddPtr(ptr.get());
  }

  void AddLambdaWithOwned(std::function<std::string()> printerFunc);
  [[nodiscard]] impl::ReferencedValueGuard AddLambdaWithRef(std::function<std::string()> printerFunc);
  // === / add messages ===

  [[nodiscard]] ResolvedContext Resolve() const;
  void Orphan() const;

  //
  // NOTE: slow
  void LogInnerExecution();
  bool IsLoggingInnerExecution() const;

  // XXX: move to some common headedr?
  constexpr static size_t kSize = 1024;

private:
  third_party::userver::utils::FastPimpl<OnstackContextImpl, kSize, 8, /*Strict=*/true> pImpl;

  friend ResolvedContext;
};

#define ANTIGO_CONTEXT_INIT(ctx) ::Antigo::OnstackContext ctx(__FILE__, __LINE__, __func__)

} // namespace Antigo
