#include "EspmReader.h"
#include "fmt/format.h"

std::shared_ptr<EspmReader> EspmReader::GetEspmReader(
  espm::CompressedFieldsCache& espmCache, const espm::CombineBrowser& browser)
{
  return std::make_unique<EspmReader>(EspmReader(espmCache, browser));
}

EspmReader::EspmReader(espm::CompressedFieldsCache& espmCache,
                       const espm::CombineBrowser& browser)
  : espmCache(espmCache)
  , browser(browser)
{
}

espm::RACE::Data EspmReader::GetRaceData(const uint32_t raceId)
{
  const auto lookUpRace = browser.LookupById(raceId);
  if (!lookUpRace.rec || lookUpRace.rec->GetType() != "RACE") {
    throw std::runtime_error(
      fmt::format("Unable to get race from {0:x}", raceId));
  }
  return espm::Convert<espm::RACE>(lookUpRace.rec)->GetData(espmCache);
}

espm::WEAP::Data EspmReader::GetWeaponData(const uint32_t source)
{
  const auto lookUpWeapon = browser.LookupById(source);
  if (!lookUpWeapon.rec || lookUpWeapon.rec->GetType() != "WEAP") {
    throw std::runtime_error(
      fmt::format("Unable to get weapon from {0:x} formId", source));
  }

  return espm::Convert<espm::WEAP>(lookUpWeapon.rec)->GetData(espmCache);
}

espm::NPC_::Data EspmReader::GetNPCData(const uint32_t baseId)
{
  const auto lookUpNPC = browser.LookupById(baseId);
  if (!lookUpNPC.rec || lookUpNPC.rec->GetType() != "NPC_") {
    throw std::runtime_error(
      fmt::format("Unable to get raceId from {0:x}", baseId));
  }
  return espm::Convert<espm::NPC_>(lookUpNPC.rec)->GetData(espmCache);
}
