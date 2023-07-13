#pragma once
#include <memory>
#include <sparsepp/spp.h>
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

  spp::sparse_hash_map<const RecordHeader*, Entry> data;
};

}

#pragma pack(pop)
