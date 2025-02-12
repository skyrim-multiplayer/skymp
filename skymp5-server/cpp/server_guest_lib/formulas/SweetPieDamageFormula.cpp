#include "SweetPieDamageFormula.h"

#include <cstdint>
#include <stdexcept>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "HitData.h"
#include "MpActor.h"
#include "SpellCastData.h"
#include "libespm/Loader.h"

namespace {

bool IsPlayerBaseId(const MpActor& actor)
{
  return actor.GetBaseId() == 0x7;
}

} // namespace

SweetPieDamageFormula::SweetPieDamageFormula(
  std::unique_ptr<IDamageFormula> baseFormula_, const nlohmann::json& config)
  : baseFormula(std::move(baseFormula_))
  , settings(std::nullopt)
{
  if (config.is_object()) {
    settings = ParseConfig(config);
  }
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

  if (auto it = config.find("damageMultByLevelTargetNpc");
      it != config.end()) {
    result.damageMultByLevelTargetNpc.emplace();

    for (const auto& level : *it) {
      result.damageMultByLevelTargetNpc->push_back(level.get<float>());
    }

    if (result.damageMultByLevelTargetNpc->size() != 5) {
      throw std::runtime_error(
        "error parsing damage formula config: damageMultByLevelTargetNpc must "
        "have 5 elements");
    }
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
  const float baseDamage =
    baseFormula->CalculateDamage(aggressor, target, hitData);

  if (!settings) {
    return baseDamage;
  }

  const uint32_t weaponFormId = hitData.source;
  auto& espmCache = aggressor.GetParent()->GetEspmCache();

  const auto& keywordIds = aggressor.GetParent()
                             ->GetEspm()
                             .GetBrowser()
                             .LookupById(weaponFormId)
                             .rec->GetKeywordIds(espmCache);
  std::vector<const char*> keywordNames;
  keywordNames.reserve(keywordIds.size());
  for (const uint32_t id : keywordIds) {
    const auto& keyword = espm::GetData<espm::KYWD>(id, aggressor.GetParent());
    keywordNames.emplace_back(keyword.editorId);
  }
  spdlog::debug("SweetPieDamageFormula: {:x} hit {:x}, weapon keywords: [{}]",
                aggressor.GetFormId(), target.GetFormId(),
                fmt::join(keywordNames.begin(), keywordNames.end(), ","));

  const auto& levelMapping =
    !IsPlayerBaseId(target) && settings->damageMultByLevelTargetNpc
    ? *settings->damageMultByLevelTargetNpc
    : settings->damageMultByLevel;
  for (const auto& keyword : keywordNames) {
    const auto it = settings->weaponKeywords.find(keyword);
    if (it == settings->weaponKeywords.end()) {
      continue;
    }
    const auto& levelItemsFormIds = it->second;
    for (int lvl = 4; lvl >= 1; --lvl) {
      if (aggressor.GetInventory().HasItem(levelItemsFormIds[lvl - 1])) {
        return baseDamage * levelMapping[lvl];
      }
    }
    return baseDamage * levelMapping[0];
  }

  return baseDamage;
}

float SweetPieDamageFormula::CalculateDamage(
  const MpActor& aggressor, const MpActor& target,
  const SpellCastData& spellCastData) const
{
  return baseFormula->CalculateDamage(aggressor, target, spellCastData);
}
