#include "StringUtils.h"

bool Viet::StartsWith(const std::string &str, const std::string &prefix) {
  return str.compare(0, prefix.size(), prefix) == 0;
}

bool Viet::StartsWith(const std::string &str, const char *prefix) {
  return str.compare(0, strlen(prefix), prefix) == 0;
}
