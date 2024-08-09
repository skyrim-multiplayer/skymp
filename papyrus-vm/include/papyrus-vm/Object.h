#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "VarValue.h"

class VarInfoView
{
public:
  VarInfoView(const uint8_t* data_, size_t dataSize_,
              const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  const std::string& GetName() const noexcept;
  const std::string& GetTypeName() const noexcept;
  uint32_t GetUserFlags() const noexcept;
  VarValue GetValue() const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

class PropInfoView
{
public:
  PropInfoView(const uint8_t* data_, size_t dataSize_,
               const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  enum
  {
    kFlags_Read = 1 << 0,
    kFlags_Write = 1 << 1,
    kFlags_AutoVar = 1 << 2,
  };

  const std::string& GetName() const noexcept;
  const std::string& GetType() const noexcept;
  const std::string& GetDocString() const noexcept;
  uint32_t GetUserFlags() const noexcept;
  uint8_t GetFlags() const noexcept;
  const std::string& GetAutoVarName() const noexcept;
  const FunctionInfoView& GetReadHandler() const noexcept;
  const FunctionInfoView& GetWriteHandler() const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

class ParamInfoView
{
public:
  ParamInfoView(const uint8_t* data_, size_t dataSize_,
                const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  const std::string& GetName() const noexcept;
  const std::string& GetType() const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

class InstructionView
{
public:
  InstructionView(const uint8_t* data_, size_t dataSize_,
                  const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  uint8_t GetOp() const noexcept;

  template <class Callback /* void(const VarValue&) */>
  void ForEachArg(const Callback& callback) const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

class StateFunctionView
{
public:
  StateFunctionView(const uint8_t* data_, size_t dataSize_,
                    const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  const std::string& GetName() const noexcept;
  const FunctionInfoView& GetFunction() const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

class StateInfoView
{
public:
  StateInfoView(const uint8_t* data_, size_t dataSize_,
                const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  const std::string& GetName() const noexcept;

  template <class Callback /* void(const StateFunctionView&) */>
  void ForEachFunction(const Callback& callback) const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

class ObjectView
{
public:
  ObjectView(const uint8_t* data_, size_t dataSize_,
             const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  const std::string& GetName() const noexcept;
  const std::string& GetParentClassName() const noexcept;
  const std::string& GetDocString() const noexcept;
  uint32_t GetUserFlags() const noexcept;
  const std::string& GetAutoStateName() const noexcept;

  template <class Callback /* void(const VarInfoView&) */>
  void ForEachVariable(const Callback& callback) const noexcept;

  template <class Callback /* void(const StateInfoView&) */>
  void ForEachState(const Callback& callback) const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

// struct Object
// {
//   std::string NameIndex;

//   std::string parentClassName;
//   std::string docstring;
//   uint32_t userFlags = 0;
//   std::string autoStateName;

//   struct VarInfo
//   {
//     std::string name;
//     std::string typeName;
//     uint32_t userFlags = 0;
//     VarValue value = VarValue();
//   };

//   struct PropInfo
//   {
//     enum
//     {
//       kFlags_Read = 1 << 0,
//       kFlags_Write = 1 << 1,
//       kFlags_AutoVar = 1 << 2,
//     };

//     std::string name;
//     std::string type;
//     std::string docstring;
//     uint32_t userFlags = 0;
//     uint8_t flags = 0; // 1 and 2 are read/write
//     std::string autoVarName;

//     FunctionInfo readHandler;
//     FunctionInfo writeHandler;
//   };

//   struct StateInfo
//   {

//     struct StateFunction
//     {
//       std::string name;
//       FunctionInfo function;
//     };

//     std::string name;

//     std::vector<StateFunction> functions;
//   };

//   std::vector<VarInfo> variables;

//   std::vector<PropInfo> properties;

//   std::vector<StateInfo> states;
// };
