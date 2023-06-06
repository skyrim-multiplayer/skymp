#include "SweetPieDamageFormula.h"

#include <cstdint>
#include <spdlog/spdlog.h>
#include <stdexcept>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "HitData.h"
#include "MpActor.h"
#include "libespm/Loader.h"
#include "libespm/espm.h"

SweetPieDamageFormula::SweetPieDamageFormula(
  std::unique_ptr<IDamageFormula> baseFormula_, const nlohmann::json& config)
  : baseFormula(std::move(baseFormula_))
  , settings(ParseConfig(config))
{
}

SweetPieDamageFormulaSettings SweetPieDamageFormula::ParseConfig(
  const nlohmann::json& config) const
{
  SweetPieDamageFormulaSettings result{};
  for (const auto& level : config["damageMultByLevel"]) {
    result.damageMultByLevel.push_back(level.get<float>());
  }
  if (result.damageMultByLevel.size() != 5) {
    throw std::runtime_error("error parsing damage formula config: "
                             "damageMultByLevel must have 5 elements");
  }

  for (const auto& keywordData : config["weaponKeywords"].items()) {
    const auto& keyword = keywordData.key();
    const auto& levelItems = keywordData.value()["levelItems"];

    std::vector<uint32_t>& formIds = result.weaponKeywords[keyword];
    for (const auto& formId : levelItems) {
      if (formId.is_string()) {
        formIds.push_back(std::stoul(formId.get<std::string>(), nullptr, 16));
      } else if (formId.is_number_integer()) {
        formIds.push_back(formId.get<uint32_t>());
      } else {
        throw std::runtime_error(fmt::format(
          "error parsing damage formula config: invalid formId type for "
          "keyword {}: {} ({})",
          keyword, formId.type_name(), static_cast<int>(formId.type())));
      }
    }
    if (formIds.size() != 4) {
      throw std::runtime_error(
        fmt::format("error parsing damage formula config: got {} formIds for "
                    "keyword {} instead of 4",
                    formIds.size(), keyword));
    }
  }
  return result;
}

float SweetPieDamageFormula::CalculateDamage(const MpActor& aggressor,
                                             const MpActor& target,
                                             const HitData& hitData) const
{
  float baseDamage = baseFormula->CalculateDamage(aggressor, target, hitData);

  uint32_t weaponFormId = hitData.source;
  auto& espmCache = aggressor.GetParent()->GetEspmCache();

  const auto& keywordIds = aggressor.GetParent()
                             ->GetEspm()
                             .GetBrowser()
                             .LookupById(weaponFormId)
                             .rec->GetKeywordIds(espmCache);
  std::vector<const char*> keywordNames;
  keywordNames.reserve(keywordIds.size());
  for (uint32_t id : keywordIds) {
    const auto& keyword = espm::GetData<espm::KYWD>(id, aggressor.GetParent());
    keywordNames.emplace_back(keyword.editorId);
  }
  spdlog::debug("SweetPieDamageFormula: {:x} hit {:x}, weapon keywords: [{}]",
                aggressor.GetFormId(), target.GetFormId(),
                fmt::join(keywordNames.begin(), keywordNames.end(), ","));

  for (const auto& keyword : keywordNames) {
    const auto it =
      settings.weaponKeywords.find(keyword); // TODO: rename weaponKeywords?
    if (it == settings.weaponKeywords.end()) {
      continue;
    }
    const auto& levelItemsFormIds = it->second;
    for (int lvl = 4; lvl >= 1; --lvl) {
      if (aggressor.GetInventory().HasItem(levelItemsFormIds[lvl - 1])) {
        return baseDamage * settings.damageMultByLevel[lvl];
      }
    }
    return baseDamage * settings.damageMultByLevel[0];
  }

  return baseDamage;
}
