#include "Loader.h"
#include "espm.h"

class EspmReader
{
public:
  ~EspmReader() = default;
  static std::shared_ptr<EspmReader> GetEspmReader(
    espm::CompressedFieldsCache& espmCache,
    const espm::CombineBrowser& browser);

  espm::RACE::Data GetRaceData(const uint32_t raceId);
  espm::WEAP::Data GetWeaponData(const uint32_t source);
  espm::NPC_::Data GetNPCData(const uint32_t baseId);

private:
  EspmReader(espm::CompressedFieldsCache& espmCache,
             const espm::CombineBrowser& browser);
  espm::CompressedFieldsCache& espmCache;
  const espm::CombineBrowser& browser;
};
