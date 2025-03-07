#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Antigo {

struct SourceLocationEntry {
  const char* filename;
  uint16_t line;
  const char* func;
};

struct ResolvedMessageEntry {
  std::string type;
  std::string strval;
};

struct ResolvedContextEntry {
  SourceLocationEntry sourceLoc;
  std::vector<ResolvedMessageEntry> messages; // 20241218 1 common buffer (for each thread)
};

struct ResolvedContext {
  std::vector<ResolvedContextEntry> entries;
  std::string reason;

  std::string ToString() const;
  void Print() const;
};

std::ostream& operator<<(std::ostream& os, const ResolvedContextEntry& entry);
std::ostream& operator<<(std::ostream& os, const ResolvedContext& self);

}
