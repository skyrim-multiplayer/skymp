#include "MpChangeForms.h"
#include "JsonUtils.h"

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

  res["healthPercentage"] = changeForm.healthPercentage;
  res["magickaPercentage"] = changeForm.magickaPercentage;
  res["staminaPercentage"] = changeForm.staminaPercentage;

  res["isDead"] = changeForm.isDead;
  return res;
}

MpChangeForm MpChangeForm::JsonToChangeForm(simdjson::dom::element& element)
{
  static const JsonPointer recType("recType"), formDesc("formDesc"),
    baseDesc("baseDesc"), position("position"), angle("angle"),
    worldOrCellDesc("worldOrCellDesc"), inv("inv"), isHarvested("isHarvested"),
    isOpen("isOpen"), baseContainerAdded("baseContainerAdded"),
    nextRelootDatetime("nextRelootDatetime"), isDisabled("isDisabled"),
    profileId("profileId"), isRaceMenuOpen("isRaceMenuOpen"),
    appearanceDump("appearanceDump"), equipmentDump("equipmentDump"),
    dynamicFields("dynamicFields"), healthPercentage("healthPercentage"),
    magickaPercentage("magickaPercentage"),
    staminaPercentage("staminaPercentage"), isDead("isDead");

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

  ReadEx(element, healthPercentage, &res.healthPercentage);
  ReadEx(element, magickaPercentage, &res.magickaPercentage);
  ReadEx(element, staminaPercentage, &res.staminaPercentage);
  ReadEx(element, isDead, &res.isDead);

  simdjson::dom::element jDynamicFields;
  ReadEx(element, dynamicFields, &jDynamicFields);
  res.dynamicFields = DynamicFields::FromJson(nlohmann::json::parse(
    static_cast<std::string>(simdjson::minify(jDynamicFields))));

  return res;
}
