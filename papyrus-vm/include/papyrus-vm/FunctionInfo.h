#pragma once
#include <cstdint>
#include <string>
#include <vector>

class FunctionInfoView
{
public:
  FunctionInfoView(const uint8_t* data_, size_t dataSize_,
                   const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  bool IsValid() const noexcept;
  const std::string& GetReturnType() const noexcept;
  const std::string& GetDocString() const noexcept;
  uint32_t GetUserFlags() const noexcept;
  uint8_t GetFlags() const noexcept;

  template <class Callback /* void(const ParamInfoView&) */>
  void ForEachParam(const Callback& callback) const noexcept;

  template <class Callback /* void(const ParamInfoView&) */>
  void ForEachLocal(const Callback& callback) const noexcept;

  const FunctionCodeView& GetFunctionCode() const noexcept;

  bool IsGlobal() const noexcept { return GetFlags() & (1 << 0); }
  bool IsNative() const noexcept { return GetFlags() & (1 << 1); }

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

// struct FunctionInfo
// {
//   bool valid = false;

//   enum
//   {
//     kFlags_Read = 1 << 0,
//     kFlags_Write = 1 << 1,
//   };

//   struct ParamInfo
//   {
//     std::string name;
//     std::string type;
//   };

//   std::string returnType;
//   std::string docstring;
//   uint32_t userFlags = 0;
//   uint8_t flags = 0;

//   std::vector<ParamInfo> params;
//   std::vector<ParamInfo> locals;

//   FunctionCode code;

//   bool IsGlobal() const { return flags & (1 << 0); }

//   bool IsNative() const { return flags & (1 << 1); }
// };
