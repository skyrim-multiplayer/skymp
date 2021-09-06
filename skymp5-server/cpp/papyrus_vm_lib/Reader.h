#pragma once
#include "Structures.h"

class Reader
{
  std::string path = "";

  std::vector<std::shared_ptr<PexScript>> sourceStructures;

  std::shared_ptr<PexScript> structure = nullptr;

  std::vector<int> numArgumentsForOpcodes = { 0, 3, 3, 3, 3, 3,  3,  3,  3,
                                              3, 2, 2, 2, 2, 2,  3,  3,  3,
                                              3, 3, 1, 2, 2, -4, -3, -4, 1,
                                              3, 3, 3, 2, 2, 3,  3,  4,  4 };

  bool additionalArguments = false;

  int currentReadPositionInFile = 0;

  std::vector<uint8_t> arrayBytes;

  std::string FillSource();
  std::string FillUser();
  std::string FillMachine();

  ScriptHeader FillHeader();
  StringTable FillStringTable();
  DebugInfo FillDebugInfo();
  std::vector<UserFlag> FillUserFlagTable();
  std::vector<Object> FillObjectTable();

  DebugInfo::DebugFunction FillDebugFunction();
  UserFlag FillUserFlag();
  Object FillObject();
  Object::VarInfo FillVariable();
  VarValue FillVariableData();
  Object::PropInfo FillProperty();
  FunctionInfo FillFuncInfo();
  FunctionCode FillFunctionCode(int countInstructions);
  uint8_t GetCountArguments(uint8_t item);
  Object::StateInfo FillState();
  Object::StateInfo::StateFunction FillStateFunction();

  uint8_t Read8_bit();
  uint16_t Read16_bit();
  uint32_t Read32_bit();
  uint64_t Read64_bit();
  std::string ReadString(int Size);

  void Read();
  void CreateScriptStructure(const std::vector<uint8_t>& arrayBytes);

public:
  std::vector<std::shared_ptr<PexScript>> GetSourceStructures();
  Reader(const std::vector<std::string>& vectorPath);
  Reader(const std::vector<std::vector<uint8_t>>& pex);
};
