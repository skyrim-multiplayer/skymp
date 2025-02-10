#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#pragma pack(push, 1)

namespace espm {

class RecordHeader;

class CompressedFieldsCache
{
  friend class RecordHeaderAccess;
  friend class Browser;

private:
  struct Entry
  {
    std::shared_ptr<std::vector<uint8_t>> decompressedFieldsHolder;
  };

  std::unordered_map<const RecordHeader*, Entry> data;
};

}

#pragma pack(pop)
