#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "DebugInfo.h"
#include "Object.h"
#include "ScriptHeader.h"
#include "StringTable.h"
#include "UserFlag.h"

class PexScriptView
{
public:
  PexScriptView(const uint8_t* data_, size_t dataSize_,
                const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  ScriptHeaderView GetHeader() const noexcept;

  DebugInfoView GetDebugInfo() const noexcept;

  template <class Callback /* void(const UserFlagView&) */>
  void ForEachUserFlag(const Callback& callback) const noexcept;

  template <class Callback /* void(const ObjectView&) */>
  void ForEachObject(const Callback& callback) const noexcept;

  const std::string& GetSource() const noexcept;

  const std::string& GetUser() const noexcept;

  const std::string& GetMachine() const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};

class PexScript
{
public:
  struct Lazy
  {
    std::string source;
    std::function<std::shared_ptr<PexScript>()> fn;
  };

  // Copying/moving StringTable breaks VarValues with strings
  PexScript(const PexScript&) = delete;
  PexScript& operator=(const PexScript&) = delete;
  PexScript(PexScript&&) = delete;
  PexScript& operator=(PexScript&&) = delete;

  PexScript(StringTable&& stringTable_, std::vector<uint8_t>&& data_) noexcept
    : stringTable(std::move(stringTable_))
    , data(std::move(data_))
  {
  }

  PexScriptView GetView() const noexcept
  {
    return PexScriptView(data.data(), data.size(), &stringTable);
  }

private:
  StringTable stringTable;
  std::vector<uint8_t> data;
};

// struct PexScript
// {
//   // Copying PexScript breaks VarValues with strings
//   PexScript() = default;
//   PexScript(const PexScript&) = delete;
//   PexScript& operator=(const PexScript&) = delete;

//   struct Lazy
//   {
//     std::string source;
//     std::function<std::shared_ptr<PexScript>()> fn;
//   };

//   ScriptHeader header;
//   StringTable stringTable;
//   DebugInfo debugInfo;
//   std::vector<UserFlag> userFlagTable;
//   std::vector<Object> objectTable;

//   std::string source;
//   std::string user;
//   std::string machine;
// };
