#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "FunctionInfo.h"
#include "VarValue.h"

struct Object
{
  std::string NameIndex;

  std::string parentClassName;
  std::string docstring;
  uint32_t userFlags = 0;
  std::string autoStateName;

  struct VarInfo
  {
    std::string name;
    std::string typeName;
    uint32_t userFlags = 0;
    VarValue value = VarValue();
  };

  struct PropInfo
  {
    enum
    {
      kFlags_Read = 1 << 0,
      kFlags_Write = 1 << 1,
      kFlags_AutoVar = 1 << 2,
    };

    std::string name;
    std::string type;
    std::string docstring;
    uint32_t userFlags = 0;
    uint8_t flags = 0; // 1 and 2 are read/write
    std::string autoVarName;

    FunctionInfo readHandler;
    FunctionInfo writeHandler;
  };

  struct StateInfo
  {

    struct StateFunction
    {
      std::string name;
      FunctionInfo function;
    };

    std::string name;

    std::vector<StateFunction> functions;
  };

  std::vector<VarInfo> variables;

  std::vector<PropInfo> properties;

  std::vector<StateInfo> states;
};
