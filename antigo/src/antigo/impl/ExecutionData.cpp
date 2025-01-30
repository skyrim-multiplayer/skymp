#include "antigo/impl/ExecutionData.h"

// XXX rename namespace, move out of impl?
namespace Antigo {

class OnstackContextImpl;

ExecutionData& GetCurrentExecutionData() {
  thread_local ExecutionData data;
  return data;
}

bool HasExceptionWitness() {
  return !GetCurrentExecutionData().errorWitnesses.empty();
}

ResolvedContext PopExceptionWitness() {
  auto w = std::move(GetCurrentExecutionData().errorWitnesses.back());
  GetCurrentExecutionData().errorWitnesses.pop_back();
  return w;
}

bool HasExceptionWitnessOrphan()
{
  return GetCurrentExecutionData().orphans.size();
}

ResolvedContext PopExceptionWitnessOrphan()
{
  auto w = std::move(GetCurrentExecutionData().orphans.back());
  GetCurrentExecutionData().orphans.pop_back();
  return w;
}

namespace impl {
bool HasCleanState() {
  auto& d = GetCurrentExecutionData();
  return d.errorWitnesses.empty() && d.orphans.empty() && d.stackCtxChain.empty();
}
}

}
