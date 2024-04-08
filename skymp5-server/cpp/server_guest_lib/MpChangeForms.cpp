#include "MpChangeForms.h"
#include "JsonUtils.h"

namespace {
std::vector<std::string> ToStringArray(const std::vector<FormDesc>& formDescs)
{
  std::vector<std::string> res(formDescs.size());
  std::transform(formDescs.begin(), formDescs.end(), res.begin(),
                 [](const FormDesc& v) { return v.ToString(); });
  return res;
}
std::vector<FormDesc> ToFormDescsArray(const std::vector<std::string>& strings)
{
  std::vector<FormDesc> res(strings.size());
  std::transform(strings.begin(), strings.end(), res.begin(),
                 [](const std::string& v) { return FormDesc::FromString(v); });
  return res;
}
}

nlohmann::json MpChangeForm::ToJson(const MpChangeForm& changeForm)
{
  auto res = nlohmann::json::object();
  res["recType"] = static_cast<int>(changeForm.recType);
  res["formDesc"] = changeForm.formDesc.ToString();
  res["baseDesc"] = changeForm.baseDesc.ToString();
  res["position"] = { changeForm.position[0], changeForm.position[1],
                      changeForm.position[2] };
  res["angle"] = { changeForm.angle[0], changeForm.angle[1],
                   changeForm.angle[2] };
  res["worldOrCellDesc"] = changeForm.worldOrCellDesc.ToString();
  res["inv"] = changeForm.inv.ToJson();
  res["isHarvested"] = changeForm.isHarvested;
  res["isOpen"] = changeForm.isOpen;
  res["baseContainerAdded"] = changeForm.baseContainerAdded;
  res["nextRelootDatetime"] = changeForm.nextRelootDatetime;
  res["isDisabled"] = changeForm.isDisabled;
  res["profileId"] = changeForm.profileId;
  res["isDeleted"] = changeForm.isDeleted;
  res["count"] = changeForm.count;
  res["isRaceMenuOpen"] = changeForm.isRaceMenuOpen;
  res["dynamicFields"] = changeForm.dynamicFields.GetAsJson();

  if (changeForm.appearanceDump.empty()) {
    res["appearanceDump"] = nullptr;
  } else {
    res["appearanceDump"] = nlohmann::json::parse(changeForm.appearanceDump);
  }

  if (changeForm.equipmentDump.empty()) {
    res["equipmentDump"] = nullptr;
  } else {
    res["equipmentDump"] = nlohmann::json::parse(changeForm.equipmentDump);
  }

  res["learnedSpells"] = changeForm.learnedSpells.GetLearnedSpells();

  res["healthPercentage"] = changeForm.actorValues.healthPercentage;
  res["magickaPercentage"] = changeForm.actorValues.magickaPercentage;
  res["staminaPercentage"] = changeForm.actorValues.staminaPercentage;

  res["isDead"] = changeForm.isDead;

  res["consoleCommandsAllowed"] = changeForm.consoleCommandsAllowed;

  res["spawnPoint_pos"] = { changeForm.spawnPoint.pos[0],
                            changeForm.spawnPoint.pos[1],
                            changeForm.spawnPoint.pos[2] };
  res["spawnPoint_rot"] = { changeForm.spawnPoint.rot[0],
                            changeForm.spawnPoint.rot[1],
                            changeForm.spawnPoint.rot[2] };
  res["spawnPoint_cellOrWorldDesc"] =
    changeForm.spawnPoint.cellOrWorldDesc.ToString();

  res["spawnDelay"] = changeForm.spawnDelay;
  res["effects"] = changeForm.activeMagicEffects.ToJson();

  if (!changeForm.templateChain.empty()) {
    res["templateChain"] = ToStringArray(changeForm.templateChain);
  }

  // TODO: uncomment when add script vars save feature
  // if (changeForm.lastAnimation.has_value()) {
  //   res["lastAnimation"] = *changeForm.lastAnimation;
  // }

  if (changeForm.setNodeTextureSet.has_value()) {
    res["setNodeTextureSet"] = *changeForm.setNodeTextureSet;
  }

  if (changeForm.setNodeScale.has_value()) {
    res["setNodeScale"] = *changeForm.setNodeScale;
  }

  if (changeForm.displayName.has_value()) {
    res["displayName"] = *changeForm.displayName;
  }

  return res;
}

MpChangeForm MpChangeForm::JsonToChangeForm(simdjson::dom::element& element)
{
  static const JsonPointer recType("recType"), formDesc("formDesc"),
    baseDesc("baseDesc"), position("position"), angle("angle"),
    worldOrCellDesc("worldOrCellDesc"), inv("inv"), isHarvested("isHarvested"),
    isOpen("isOpen"), baseContainerAdded("baseContainerAdded"),
    nextRelootDatetime("nextRelootDatetime"), isDisabled("isDisabled"),
    profileId("profileId"), isDeleted("isDeleted"), count("count"),
    isRaceMenuOpen("isRaceMenuOpen"), appearanceDump("appearanceDump"),
    equipmentDump("equipmentDump"), learnedSpells("learnedSpells"),
    dynamicFields("dynamicFields"), healthPercentage("healthPercentage"),
    magickaPercentage("magickaPercentage"),
    staminaPercentage("staminaPercentage"), isDead("isDead"),
    consoleCommandsAllowed("consoleCommandsAllowed"),
    spawnPointPos("spawnPoint_pos"), spawnPointRot("spawnPoint_rot"),
    spawnPointCellOrWorldDesc("spawnPoint_cellOrWorldDesc"),
    spawnDelay("spawnDelay"), effects("effects"),
    templateChain("templateChain"), lastAnimation("lastAnimation"),
    setNodeTextureSet("setNodeTextureSet"), setNodeScale("setNodeScale"),
    displayName("displayName");

  MpChangeForm res;
  ReadEx(element, recType, &res.recType);

  const char* tmp;
  simdjson::dom::element jTmp;

  ReadEx(element, formDesc, &tmp);
  res.formDesc = FormDesc::FromString(tmp);

  ReadEx(element, baseDesc, &tmp);
  res.baseDesc = FormDesc::FromString(tmp);

  ReadEx(element, position, &jTmp);
  for (int i = 0; i < 3; ++i) {
    ReadEx(jTmp, i, &res.position[i]);
  }

  ReadEx(element, angle, &jTmp);
  for (int i = 0; i < 3; ++i) {
    ReadEx(jTmp, i, &res.angle[i]);
  }

  ReadEx(element, worldOrCellDesc, &tmp);
  res.worldOrCellDesc = FormDesc::FromString(tmp);

  ReadEx(element, inv, &jTmp);
  res.inv = Inventory::FromJson(jTmp);

  ReadEx(element, isHarvested, &res.isHarvested);
  ReadEx(element, isOpen, &res.isOpen);
  ReadEx(element, baseContainerAdded, &res.baseContainerAdded);
  ReadEx(element, nextRelootDatetime, &res.nextRelootDatetime);
  ReadEx(element, isDisabled, &res.isDisabled);
  ReadEx(element, profileId, &res.profileId);

  if (element.at_pointer(isDeleted.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(element, isDeleted, &res.isDeleted);
  }

  if (element.at_pointer(count.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(element, count, &res.count);
  }

  ReadEx(element, isRaceMenuOpen, &res.isRaceMenuOpen);

  ReadEx(element, appearanceDump, &jTmp);
  res.appearanceDump = simdjson::minify(jTmp);
  if (res.appearanceDump == "null") {
    res.appearanceDump.clear();
  }

  ReadEx(element, equipmentDump, &jTmp);
  res.equipmentDump = simdjson::minify(jTmp);
  if (res.equipmentDump == "null") {
    res.equipmentDump.clear();
  }

  if (element.at_pointer(learnedSpells.GetData()).error() ==
      simdjson::error_code::SUCCESS) {

    std::vector<LearnedSpells::Data::key_type> learnedSpellsData;

    ReadVector(element, learnedSpells, &learnedSpellsData);

    for (const auto spellId : learnedSpellsData) {
      res.learnedSpells.LearnSpell(spellId);
    }
  }

  ReadEx(element, healthPercentage, &res.actorValues.healthPercentage);
  ReadEx(element, magickaPercentage, &res.actorValues.magickaPercentage);
  ReadEx(element, staminaPercentage, &res.actorValues.staminaPercentage);

  ReadEx(element, isDead, &res.isDead);

  if (element.at_pointer(consoleCommandsAllowed.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(element, consoleCommandsAllowed, &res.consoleCommandsAllowed);
  }

  simdjson::dom::element jDynamicFields;
  ReadEx(element, dynamicFields, &jDynamicFields);
  res.dynamicFields = DynamicFields::FromJson(nlohmann::json::parse(
    static_cast<std::string>(simdjson::minify(jDynamicFields))));

  ReadEx(element, spawnPointPos, &jTmp);
  for (int i = 0; i < 3; ++i) {
    ReadEx(jTmp, i, &res.spawnPoint.pos[i]);
  }

  ReadEx(element, spawnPointRot, &jTmp);
  for (int i = 0; i < 3; ++i) {
    ReadEx(jTmp, i, &res.spawnPoint.rot[i]);
  }

  ReadEx(element, spawnPointCellOrWorldDesc, &tmp);
  res.spawnPoint.cellOrWorldDesc = FormDesc::FromString(tmp);

  ReadEx(element, spawnDelay, &res.spawnDelay);

  if (element.at_pointer(effects.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(element, effects, &jTmp);
    res.activeMagicEffects = ActiveMagicEffectsMap::FromJson(jTmp);
  }

  if (element.at_pointer(templateChain.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    std::vector<std::string> data;
    ReadVector(element, templateChain, &data);
    res.templateChain = ToFormDescsArray(data);
  }

  if (element.at_pointer(lastAnimation.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    const char* tmp;
    ReadEx(element, lastAnimation, &tmp);
    res.lastAnimation = tmp;
  }

  if (element.at_pointer(setNodeTextureSet.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    simdjson::dom::element data;
    ReadEx(element, setNodeTextureSet, &data);

    if (res.setNodeTextureSet == std::nullopt) {
      res.setNodeTextureSet = std::map<std::string, std::string>();
    }

    for (auto [key, value] : data.get_object()) {
      std::string keyStr = key.data();
      std::string valueStr = value.get_string().value().data();
      res.setNodeTextureSet->emplace(keyStr, valueStr);
    }
  }

  if (element.at_pointer(setNodeScale.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    simdjson::dom::element data;
    ReadEx(element, setNodeScale, &data);

    if (res.setNodeScale == std::nullopt) {
      res.setNodeScale = std::map<std::string, float>();
    }

    for (auto [key, value] : data.get_object()) {
      std::string keyStr = key.data();
      float valueFloat = static_cast<float>(value.get_double().value());
      res.setNodeScale->emplace(keyStr, valueFloat);
    }
  }

  if (element.at_pointer(displayName.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    const char* tmp;
    ReadEx(element, displayName, &tmp);
    res.displayName = tmp;
  }

  return res;
}

void LearnedSpells::LearnSpell(const Data::key_type spellId)
{
  _learnedSpellIds.emplace(spellId);
}

void LearnedSpells::ForgetSpell(const Data::key_type spellId)
{
  _learnedSpellIds.erase(spellId);
}

size_t LearnedSpells::Count() const noexcept
{
  return _learnedSpellIds.size();
}

bool LearnedSpells::IsSpellLearned(const Data::key_type spellId) const
{
  return _learnedSpellIds.count(spellId) != 0;
}

std::vector<LearnedSpells::Data::key_type> LearnedSpells::GetLearnedSpells()
  const
{
  return std::vector(_learnedSpellIds.begin(), _learnedSpellIds.end());
}
