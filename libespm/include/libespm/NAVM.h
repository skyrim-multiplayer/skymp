#pragma once
#include "CellOrGridPos.h"
#include "RecordHeader.h"
#include <array>
#include <memory>

#pragma pack(push, 1)

namespace espm {

class NAVM final : public RecordHeader
{
public:
  static constexpr auto kType = "NVNM";

  class Vertices
  {
  public:
    Vertices(const void* nvnmField);

    const std::array<float, 3>* begin() const noexcept;
    const std::array<float, 3>* end() const noexcept;

  private:
    const int32_t* numVerticesPtr = nullptr;
    const std::array<float, 3>* beginPtr = nullptr;
    const void* nvnmField;
  };

  struct Data
  {
    std::unique_ptr<Vertices> vertices;
    uint32_t worldSpaceId = 0;
    CellOrGridPos cellOrGridPos = { 0 };
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(NAVM) == sizeof(RecordHeader));

}

#pragma pack(pop)
