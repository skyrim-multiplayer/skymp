#pragma once
#include "CompressedFieldsCache.h"
#include "FieldHeader.h"
#include "RecordFlags.h"
#include "RecordHeader.h"
#include "ZlibUtils.h"

namespace espm {

class RecordHeaderAccess
{
public:
  template <class T>
  static void IterateFields(const RecordHeader* rec, const T& f,
                            CompressedFieldsCache& compressedFieldsCache)
  {
    const int8_t* ptr = (reinterpret_cast<const int8_t*>(rec)) + sizeof(*rec);
    const int8_t* endPtr = ptr + rec->GetFieldsSizeSum();
    uint32_t fiDataSizeOverride = 0;

    if (rec->flags & RecordFlags::Compressed) {
      auto& decompressedFieldsHolder =
        compressedFieldsCache.data[rec].decompressedFieldsHolder;
      if (!decompressedFieldsHolder) {
        const uint32_t* decompSize = reinterpret_cast<const uint32_t*>(ptr);
        ptr += sizeof(uint32_t);

        auto out = std::make_shared<std::vector<uint8_t>>();
        out->resize(*decompSize);

        const auto inSize = rec->GetFieldsSizeSum() - sizeof(uint32_t);
        ZlibDecompress(ptr, inSize, out->data(), out->size());

        decompressedFieldsHolder = out;
      }

      ptr = reinterpret_cast<int8_t*>(decompressedFieldsHolder->data());
      endPtr = reinterpret_cast<int8_t*>(decompressedFieldsHolder->data() +
                                         decompressedFieldsHolder->size());
    }

    while (ptr < endPtr) {
      const auto fiHeader = reinterpret_cast<const FieldHeader*>(ptr);
      ptr += sizeof(FieldHeader);
      const uint32_t fiDataSize =
        fiHeader->dataSize ? fiHeader->dataSize : fiDataSizeOverride;
      const char* fiData = reinterpret_cast<const char*>(ptr);
      ptr += fiDataSize;

      if (!memcmp(fiHeader->type, "XXXX", 4)) {
        fiDataSizeOverride = *reinterpret_cast<const uint32_t*>(fiData);
      }
      f(fiHeader->type, fiDataSize, fiData);
    }
  }
};

}
