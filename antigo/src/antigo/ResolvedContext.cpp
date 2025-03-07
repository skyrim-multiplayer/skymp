#include "antigo/ResolvedContext.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string_view>

namespace Antigo {

namespace {

struct ApplyIndent {
  const std::string& s;
  // size_t indent;
  const std::string indentStr;
};

std::ostream& operator<<(std::ostream& os, const ApplyIndent& rhs) {
  std::string_view s = rhs.s;
  // std::string indentStr(rhs.indent, ' ');

  size_t pos = 0;
  while (pos < s.size()) {
    size_t next = s.find('\n', pos);
    next = next == std::string::npos ? s.size() : next + 1;
    assert(next > pos);

    if (pos != 0) {
      os << rhs.indentStr;
    }
    os << s.substr(pos, next - pos);
    pos = next;
  }

  return os;
}

}

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

std::ostream& operator<<(std::ostream& os, const ResolvedContextEntry& entry) {
  if (!entry.messages.empty()) {
    os << "  ╭--------\n";
  }
  for (size_t msgIdx = entry.messages.size(); msgIdx > 0; --msgIdx) {
    const auto& msg = entry.messages[msgIdx - 1];
    os << "  | (" << msg.type << ") " << ApplyIndent{msg.strval, "     " + std::string(msg.type.size(), ' ') + "  "} << "\n";
  }
  // os << "> " << entry.sourceLoc.func << "\n";
  if (!entry.messages.empty()) {
    os << "  ╰-- ";
  } else {
    os << "  o-- ";
  }
  os << entry.sourceLoc.filename << ":" << entry.sourceLoc.line << " - " << entry.sourceLoc.func;
  return os;
}

std::ostream& operator<<(std::ostream& os, const ResolvedContext& self) {
  os << "resolved context with " << self.entries.size() << " entries (reason=" << self.reason << "):\n";
  for (size_t entryIdx = 0; entryIdx < self.entries.size(); ++entryIdx) {
    const auto& entry = self.entries[entryIdx];
    os << entry;
    if (entryIdx + 1 != self.entries.size()) {
      os << '\n';
    }
  }
  if (self.entries.empty()) {
    os << "(empty)";
  }
  return os;
}

}
