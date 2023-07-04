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

  res["healthPercentage"] = changeForm.actorValues.healthPercentage;
  res["magickaPercentage"] = changeForm.actorValues.magickaPercentage;
  res["staminaPercentage"] = changeForm.actorValues.staminaPercentage;

  res["isDead"] = changeForm.isDead;

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
    staminaPercentage("staminaPercentage"), isDead("isDead"),
    spawnPointPos("spawnPoint_pos"), spawnPointRot("spawnPoint_rot"),
    spawnPointCellOrWorldDesc("spawnPoint_cellOrWorldDesc"),
    spawnDelay("spawnDelay"), effects("effects");

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

  ReadEx(element, healthPercentage, &res.actorValues.healthPercentage);
  ReadEx(element, magickaPercentage, &res.actorValues.magickaPercentage);
  ReadEx(element, staminaPercentage, &res.actorValues.staminaPercentage);
  ReadEx(element, isDead, &res.isDead);

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

  ReadEx(element, effects, &jTmp);
  res.activeMagicEffects = ActiveMagicEffectsMap::FromJson(jTmp);

  return res;
}
