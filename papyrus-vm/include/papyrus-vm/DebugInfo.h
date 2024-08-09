#pragma once
#include <cstdint>
#include <string>
#include <vector>

class DebugFunctionView
{
public:
  DebugFunctionView(const uint8_t* data_, size_t dataSize_,
                    const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  const std::string& GetObjName() const noexcept;
  const std::string& GetStateName() const noexcept;
  const std::string& GetFnName() const noexcept;
  uint8_t GetType() const noexcept;
  const std::vector<uint16_t>& GetLineNumbers()
    const noexcept; // one per instruction

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

class DebugInfoView
{
public:
  DebugInfoView(const uint8_t* data_, size_t dataSize_,
                const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  uint8_t GetFlags() const noexcept;
  uint64_t GetSourceModificationTime() const noexcept;

  template <class Callback /* void(const DebugFunctionView&) */>
  void ForEachFunction(const Callback& callback) const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

// struct DebugInfo
// {
//   uint8_t m_flags = 0;
//   uint64_t m_sourceModificationTime = 0;

//   struct DebugFunction
//   {
//     std::string objName;
//     std::string stateName;
//     std::string fnName;
//     uint8_t type = 0; // 0-3 valid

//     std::vector<uint16_t> lineNumbers; // one per instruction

//     size_t GetNumInstructions() { return lineNumbers.size(); }
//   };

//   std::vector<DebugFunction> m_data;
// };
