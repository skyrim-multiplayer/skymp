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
  res["worldOrCell"] = changeForm.worldOrCell;
  res["inv"] = changeForm.inv.ToJson();
  res["isHarvested"] = changeForm.isHarvested;
  res["isOpen"] = changeForm.isOpen;
  res["baseContainerAdded"] = changeForm.baseContainerAdded;
  res["nextRelootDatetime"] = changeForm.nextRelootDatetime;
  res["isDisabled"] = changeForm.isDisabled;
  res["profileId"] = changeForm.profileId;
  res["isRaceMenuOpen"] = changeForm.isRaceMenuOpen;
  res["dynamicFields"] = changeForm.dynamicFields.GetAsJson();

  if (changeForm.lookDump.empty()) {
    res["lookDump"] = nullptr;
  } else {
    res["lookDump"] = nlohmann::json::parse(changeForm.lookDump);
  }

  if (changeForm.equipmentDump.empty()) {
    res["equipmentDump"] = nullptr;
  } else {
    res["equipmentDump"] = nlohmann::json::parse(changeForm.equipmentDump);
  }

  res["healthPercentage"] = changeForm.healthPercentage;
  res["magickaPercentage"] = changeForm.magickaPercentage;
  res["staminaPercentage"] = changeForm.staminaPercentage;

  return res;
}

MpChangeForm MpChangeForm::JsonToChangeForm(simdjson::dom::element& element)
{
  static const JsonPointer recType("recType"), formDesc("formDesc"),
    baseDesc("baseDesc"), position("position"), angle("angle"),
    worldOrCell("worldOrCell"), inv("inv"), isHarvested("isHarvested"),
    isOpen("isOpen"), baseContainerAdded("baseContainerAdded"),
    nextRelootDatetime("nextRelootDatetime"), isDisabled("isDisabled"),
    profileId("profileId"), isRaceMenuOpen("isRaceMenuOpen"),
    lookDump("lookDump"), equipmentDump("equipmentDump"),
    dynamicFields("dynamicFields"), healthPercentage("healthPercentage"),
    magickaPercentage("magickaPercentage"),
    staminaPercentage("staminaPercentage");

  MpChangeForm res;
  ReadEx(element, recType, &res.recType);

  const char* tmp;
  simdjson::dom::element jTmp;

  ReadEx(element, formDesc, &tmp);
  res.formDesc = FormDesc::FromString(tmp);

  ReadEx(element, baseDesc, &tmp);
  res.baseDesc = FormDesc::FromString(tmp);

  ReadEx(element, position, &jTmp);
  for (int i = 0; i < 3; ++i)
    ReadEx(jTmp, i, &res.position[i]);

  ReadEx(element, angle, &jTmp);
  for (int i = 0; i < 3; ++i)
    ReadEx(jTmp, i, &res.angle[i]);

  ReadEx(element, worldOrCell, &res.worldOrCell);

  ReadEx(element, inv, &jTmp);
  res.inv = Inventory::FromJson(jTmp);

  ReadEx(element, isHarvested, &res.isHarvested);
  ReadEx(element, isOpen, &res.isOpen);
  ReadEx(element, baseContainerAdded, &res.baseContainerAdded);
  ReadEx(element, nextRelootDatetime, &res.nextRelootDatetime);
  ReadEx(element, isDisabled, &res.isDisabled);
  ReadEx(element, profileId, &res.profileId);
  ReadEx(element, isRaceMenuOpen, &res.isRaceMenuOpen);

  ReadEx(element, lookDump, &jTmp);
  res.lookDump = simdjson::minify(jTmp);
  if (res.lookDump == "null")
    res.lookDump.clear();

  ReadEx(element, equipmentDump, &jTmp);
  res.equipmentDump = simdjson::minify(jTmp);
  if (res.equipmentDump == "null")
    res.equipmentDump.clear();

  try {
    ReadEx(element, healthPercentage, &res.healthPercentage);
    ReadEx(element, magickaPercentage, &res.magickaPercentage);
    ReadEx(element, staminaPercentage, &res.staminaPercentage);
  } catch (JsonIndexException&) {
  } catch (...) {
    throw;
  }

  try {
    simdjson::dom::element jDynamicFields;
    ReadEx(element, dynamicFields, &jDynamicFields);
    res.dynamicFields = DynamicFields::FromJson(nlohmann::json::parse(
      static_cast<std::string>(simdjson::minify(jDynamicFields))));
  } catch (JsonIndexException&) {
  } catch (...) {
    throw;
  }

  return res;
}
