#pragma once

#include "antigo/Context.h"
#include "antigo/ResolvedContext.h"
#include <vector>

namespace Antigo {

class OnstackContextImpl;

struct ExecutionData
{
  std::vector<OnstackContextImpl*> stackCtxChain;
  std::vector<ResolvedContext> errorWitnesses;
  std::vector<ResolvedContext> orphans;

  std::size_t ticker = 0;
};

ExecutionData& GetCurrentExecutionData();

}
