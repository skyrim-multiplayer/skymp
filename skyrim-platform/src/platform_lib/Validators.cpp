#include "Validators.h"

bool ValidateFilename(std::string_view filename, bool allowDots)
{
  for (char c : filename) {
    if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') ||
          ('a' <= c && c <= 'z') || (c == '.' && allowDots) || c == '-' ||
          c == '_')) {
      return false;
    }
  }
  return true;
}

bool ValidateRelativePath(std::string_view path)
{
  if (path.empty() || path[0] == '/' || path[0] == '\\' ||
      path.find("..") != std::string::npos ||
      path.find(':') != std::string::npos ||
      path.find('\0') != std::string::npos) {
    return false;
  }
  return true;
}
