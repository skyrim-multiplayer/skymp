#include "antigo/Context.h"

#include <cassert>
#include <cstddef>
#include <string>

#include <cpptrace/cpptrace.hpp>

#include "antigo/impl/complex_values.h"
#include "antigo/impl/ExecutionData.h"
#include "impl/OnstackContextImpl.h"

namespace Antigo {

OnstackContext::OnstackContext(const char* filename_, size_t linenum_, const char* funcname_) : pImpl(filename_, linenum_, funcname_) {}
OnstackContext::~OnstackContext() = default;

void OnstackContext::AddMessage(const char* message) { return pImpl->AddMessage(message); }

void OnstackContext::AddPtr(const void* ptr) { return pImpl->AddPtr(ptr); }
void OnstackContext::AddUnsigned(uint64_t val) { return pImpl->AddUnsigned(val); }
void OnstackContext::AddSigned(int64_t val) { return pImpl->AddSigned(val); }

void OnstackContext::AddLambdaWithOwned(std::function<std::string()> printerFunc) { return pImpl->AddLambdaWithOwned(std::move(printerFunc)); }
impl::ReferencedValueGuard OnstackContext::AddLambdaWithRef(std::function<std::string()> printerFunc) { return pImpl->AddLambdaWithRef(std::move(printerFunc)); }

ResolvedContext OnstackContext::Resolve() const { return pImpl->Resolve(); }
void OnstackContext::Orphan() const { return pImpl->Orphan(); }

void OnstackContext::LogInnerExecution() { return pImpl->LogInnerExecution(); };
bool OnstackContext::IsLoggingInnerExecution() const { return pImpl->IsLoggingInnerExecution(); }

} // namespace Antigo
