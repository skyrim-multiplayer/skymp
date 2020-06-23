#include "JsonUtils.h"

const char* JsonIndexException::what() const
{
  return str.data();
}