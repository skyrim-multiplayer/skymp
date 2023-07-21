#pragma once
#include <cstdint>
#include <vector>

#pragma pack(push, 1)

namespace espm {

class RecordHeader;
class CompressedFieldsCache;

struct Effects
{
public:
  Effects();
  Effects(const RecordHeader* parent);
  const espm::RecordHeader* parent = nullptr;

  struct Effect
  {
    uint32_t effectId = 0; // Corresponding MGEF record id
    float magnitude = 0.f;
    uint32_t areaOfEffect = 0;
    uint32_t duration = 0;
  };

  struct Data
  {
    std::vector<Effect> effects;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

}

#pragma pack(pop)
