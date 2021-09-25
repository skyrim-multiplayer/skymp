#pragma once
#include <stdexcept>
#include <string>

class PublicError : public std::runtime_error
{
public:
  PublicError(const std::string& what)
    : std::runtime_error(what)
  {
  }
};
