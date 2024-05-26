#pragma once
#include <memory>
#include <string>
#include <vector>

#include <BitStream.h>


// class StringTable
// {
// public:
//   // Do NOT mutate storage after pex loading. reallocation would make string
//   // VarValues invalid
//   void SetStorage(std::vector<std::string> newStorage)
//   {
//     storage = std::move(newStorage);
//   }

//   std::vector<std::shared_ptr<std::string>> instanceStringTable;

//   const std::vector<std::string>& GetStorage() const { return storage; }

// private:
//   std::vector<std::string> storage;
// };

struct StringTableCache
{
  std::vector<std::string> storage;
};

class StringTableView
{
public:
  StringTableView(const uint8_t* data_, size_t dataSize_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
  }

  const std::string& GetNthString(size_t index,
                                  StringTableCache& cache) const noexcept
  {

    if (cache.storage.empty()) {
      SLNet::BitStream bs(const_cast<uint8_t*>(data), dataSize, false);

      uint16_t numStrings = 0;
      bs.Read(numStrings);

      std::vector<std::string> strings;
      strings.resize(numStrings);

      for (size_t i = 0; i < numStrings && bs.GetNumberOfUnreadBits() > 0; ++ i) {
        // TODO
      }
    }
  }

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
};
