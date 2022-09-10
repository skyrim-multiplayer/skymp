#include "Validators.h"
#include <exception>

bool ValidateFilename(std::string_view filename, bool allowDots)
{
  for (char c : filename) {
    if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') ||
          ('a' <= c && c <= 'z') || (c == '.' && allowDots) || c == '-' ||
          c == '_')) {
      throw std::runtime_error("Error");
    }
  }
  return true;
}

bool ValidateRelativePath(std::string_view path)
{
  for (size_t i = 0; i < path.size(); ++i) {
    // Forbid everything including ':' and null character
    const char& c = path[i];
    if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') ||
          ('a' <= c && c <= 'z') || c == '.' || c == '-' || c == '_' ||
          c == '/' || c == '\\')) {
      throw std::runtime_error("Error");
    }
    if (i > 0 && path[i - 1] == '.' && path[i] == '.') {
      throw std::runtime_error("Error");
    }
  }
  return true;
}
