#include "antigo/ExecutionData.h"

#include <iostream>
#include <sstream>

namespace Antigo {

struct ExecutionData {};

ExecutionData& GetCurrentExecutionData() { thread_local ExecutionData g_stub; return g_stub; }

bool HasExceptionWitness() { return false; }
ResolvedContext PopExceptionWitness() {return {};}

bool HasExceptionWitnessOrphan() { return false; }
ResolvedContext PopExceptionWitnessOrphan() {return {};}

std::string ResolvedContext::ToString() const {
  std::stringstream ss;
  ss << *this;
  if (!ss.good()) {
    return "stringstream error!";
  }
  return std::move(ss).str();
}

void ResolvedContext::Print() const {
  std::cout << *this << "\n";
}
std::ostream& operator<<(std::ostream& os, const ResolvedContext& self) {
  return os << "(no context in this build)";
}

}
