#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct FunctionInfo
{
  bool valid = false;

  enum
  {
    kFlags_Read = 1 << 0,
    kFlags_Write = 1 << 1,
  };

  struct ParamInfo
  {
    std::string name;
    std::string type;
  };

  std::string returnType;
  std::string docstring;
  uint32_t userFlags = 0;
  uint8_t flags = 0;

  std::vector<ParamInfo> params;
  std::vector<ParamInfo> locals;

  FunctionCode code;

  bool IsGlobal() const { return flags & (1 << 0); }

  bool IsNative() const { return flags & (1 << 1); }
};
