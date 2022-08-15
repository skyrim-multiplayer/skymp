#include "FlActor.h"
#include "FormDesc.h"
#include "FunctionsLibApi.h"

void RegisterActorApi(std::shared_ptr<PartOne> partOne)
{
  JsValue globalObject = JsValue::GlobalObject();

  // Actor ctor
  globalObject.SetProperty(
    "Actor", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return ActorCtor(partOne, args);
    }));

  JsValue actor = globalObject.GetProperty("Actor");
  actor.SetProperty(
    "prototype",
    globalObject.GetProperty("ObjectReference").GetProperty("prototype"));

  JsValue actorPrototype = actor.GetProperty("prototype");

  actorPrototype.SetProperty(
    "DamageActorValue",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return DamageActorValue(partOne, args);
    }));

  actorPrototype.SetProperty(
    "GetAppearance",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetAppearance(partOne, args);
    }));

  actorPrototype.SetProperty(
    "GetBaseActorValues",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetBaseActorValues(partOne, args);
    }));

  actorPrototype.SetProperty(
    "GetBounds", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetBounds(partOne, args);
    }));

  actorPrototype.SetProperty(
    "GetEquipment",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetEquipment(partOne, args);
    }));

  actorPrototype.SetProperty(
    "GetMaximumActorValues",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetMaximumActorValues(partOne, args);
    }));

  actorPrototype.SetProperty(
    "GetRaceId", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetRaceId(partOne, args);
    }));

  actorPrototype.SetProperty(
    "IsDead", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return IsDead(partOne, args);
    }));

  actorPrototype.SetProperty(
    "IsRaceMenuOpen",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return IsRaceMenuOpen(partOne, args);
    }));

  actorPrototype.SetProperty(
    "IsRespawning",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return IsRespawning(partOne, args);
    }));

  actorPrototype.SetProperty(
    "IsWeaponDrawn",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return IsWeaponDrawn(partOne, args);
    }));

  actorPrototype.SetProperty(
    "Kill", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return Kill(partOne, args);
    }));

  actorPrototype.SetProperty(
    "Respawn", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return Respawn(partOne, args);
    }));

  actorPrototype.SetProperty(
    "RestoreActorValue",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return RestoreActorValue(partOne, args);
    }));

  actorPrototype.SetProperty(
    "SetActorValuesPercentages",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return SetActorValuesPercentages(partOne, args);
    }));

  actorPrototype.SetProperty(
    "SetRaceMenuOpen",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return SetRaceMenuOpen(partOne, args);
    }));

  actorPrototype.SetProperty(
    "SetSpawnPoint",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return SetSpawnPoint(partOne, args);
    }));

  actorPrototype.SetProperty(
    "GetSpawnPoint",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetSpawnPoint(partOne, args);
    }));

  actorPrototype.SetProperty(
    "GetRespawnTime",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetRespawnTime(partOne, args);
    }));

  actorPrototype.SetProperty(
    "SetRespawnTime",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return SetRespawnTime(partOne, args);
    }));
}

JsValue ActorCtor(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[1]);
  if (formId == 0) {
    partOne->GetLogger().error("Error on check formId");
    return JsValue::Undefined();
  }

  args[0].SetProperty("_formId", args[1]);

  if (formId >= 0xff000000) {
    return JsValue::Undefined();
  }

  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);

  if (!lookupRes.rec) {
    auto type = lookupRes.rec->GetType().ToString();

    if (type != "NPC_" && type != "ACHR") {
      partOne->GetLogger().error("Actor not exists");
      return JsValue::Undefined();
    }
  }

  return JsValue::Undefined();
}

JsValue DamageActorValue(std::shared_ptr<PartOne> partOne,
                         const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto av = static_cast<espm::ActorValue>(Uint32FromJsValue(args[1]));
  auto value = static_cast<double>(args[2]);

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  actor.DamageActorValue(av, value);

  return JsValue::Undefined();
}

JsValue GetAppearance(std::shared_ptr<PartOne> partOne,
                      const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto av = static_cast<espm::ActorValue>(Uint32FromJsValue(args[1]));
  auto value = static_cast<double>(args[2]);

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  auto appearance = actor.GetAppearance();

  auto jsAppearance = JsValue::Object();

  jsAppearance.SetProperty("isFemale", JsValue::Bool(appearance->isFemale));
  jsAppearance.SetProperty("raceId", JsValue::Int(appearance->raceId));
  jsAppearance.SetProperty("weight", JsValue::Int(appearance->weight));
  jsAppearance.SetProperty("skinColor", JsValue::Int(appearance->skinColor));
  jsAppearance.SetProperty("hairColor", JsValue::Int(appearance->hairColor));

  auto headpartIds = JsValue::Array(appearance->headpartIds.size());

  for (int i = 0; i < appearance->headpartIds.size(); i++) {
    headpartIds.GetProperty("push").Call(
      { JsValue::Int(appearance->headpartIds[i]) });
  }

  jsAppearance.SetProperty("headpartIds", headpartIds);

  jsAppearance.SetProperty("headTextureSetId",
                           JsValue::Int(appearance->headTextureSetId));

  auto faceMorphs = JsValue::Array(appearance->faceMorphs.size());

  for (int i = 0; i < appearance->faceMorphs.size(); i++) {
    faceMorphs.GetProperty("push").Call(
      { JsValue::Int(appearance->faceMorphs[i]) });
  }

  jsAppearance.SetProperty("faceMorphs", faceMorphs);

  auto facePresets = JsValue::Array(appearance->facePresets.size());

  for (int i = 0; i < appearance->facePresets.size(); i++) {
    facePresets.GetProperty("push").Call(
      { JsValue::Int(appearance->facePresets[i]) });
  }

  jsAppearance.SetProperty("facePresets", facePresets);

  auto tints = JsValue::Array(appearance->tints.size());

  for (int i = 0; i < appearance->tints.size(); i++) {
    auto tint = JsValue::Object();

    tint.SetProperty("argb", JsValue::Int(appearance->tints[i].argb));
    tint.SetProperty("texturePath",
                     JsValue::String(appearance->tints[i].texturePath));
    tint.SetProperty("type", JsValue::Int(appearance->tints[i].type));

    tints.GetProperty("push").Call({ tint });
  }

  jsAppearance.SetProperty("tints", tints);
  jsAppearance.SetProperty("name", appearance->name);

  return jsAppearance;
}

JsValue GetBaseActorValues(std::shared_ptr<PartOne> partOne,
                           const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  auto baseActorValues = actor.GetBaseValues();

  JsValue jsBaseActorValues = JsValue::Object();
  jsBaseActorValues.SetProperty("health",
                                JsValue::Int(baseActorValues.health));

  jsBaseActorValues.SetProperty("magicka",
                                JsValue::Int(baseActorValues.magicka));

  jsBaseActorValues.SetProperty("stamina",
                                JsValue::Int(baseActorValues.stamina));

  jsBaseActorValues.SetProperty("healRate",
                                JsValue::Int(baseActorValues.healRate));

  jsBaseActorValues.SetProperty("magickaRate",
                                JsValue::Int(baseActorValues.magickaRate));

  jsBaseActorValues.SetProperty("staminaRate",
                                JsValue::Int(baseActorValues.staminaRate));

  jsBaseActorValues.SetProperty("healRateMult",
                                JsValue::Int(baseActorValues.healRateMult));

  jsBaseActorValues.SetProperty("magickaRateMult",
                                JsValue::Int(baseActorValues.magickaRateMult));

  jsBaseActorValues.SetProperty("staminaRateMult",
                                JsValue::Int(baseActorValues.staminaRateMult));

  return jsBaseActorValues;
}

JsValue GetBounds(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  auto bounds = actor.GetBounds();

  JsValue jsBounds = JsValue::Object();

  JsValue pos1 = JsValue::Array(3);
  JsValue pos2 = JsValue::Array(3);

  for (int i = 0; i < 3; i++) {
    pos1.GetProperty("push").Call({ JsValue::Int(bounds.pos1[i]) });
    pos2.GetProperty("push").Call({ JsValue::Int(bounds.pos2[i]) });
  }

  jsBounds.SetProperty("pos1", pos1);
  jsBounds.SetProperty("pos2", pos2);

  return jsBounds;
}

JsValue GetEquipment(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  auto equipment = actor.GetEquipment();

  JsValue jsEquipment = JsValue::Object();

  auto inventory = JsValue::Object();

  auto entries = JsValue::Array(equipment.inv.GetTotalItemCount());

  for (int i = 0; i < equipment.inv.entries.size(); i++) {
    auto item = JsValue::Object();

    item.SetProperty("baseId", JsValue::Int(equipment.inv.entries[i].baseId));
    item.SetProperty("count", JsValue::Int(equipment.inv.entries[i].count));
    item.SetProperty("worn",
                     JsValue::Int(int(equipment.inv.entries[i].extra.worn)));

    entries.SetProperty(i, item);
  }

  inventory.SetProperty("entries", entries);
  jsEquipment.SetProperty("inv", inventory);
  jsEquipment.SetProperty("numChanges", JsValue::Int(equipment.numChanges));

  return jsEquipment;
}

JsValue GetMaximumActorValues(std::shared_ptr<PartOne> partOne,
                              const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  auto maximumValues = actor.GetMaximumValues();

  JsValue jsMaximumValues = JsValue::Object();
  jsMaximumValues.SetProperty("health", JsValue::Int(maximumValues.health));

  jsMaximumValues.SetProperty("magicka", JsValue::Int(maximumValues.magicka));

  jsMaximumValues.SetProperty("stamina", JsValue::Int(maximumValues.stamina));

  jsMaximumValues.SetProperty("healRate",
                              JsValue::Int(maximumValues.healRate));

  jsMaximumValues.SetProperty("magickaRate",
                              JsValue::Int(maximumValues.magickaRate));

  jsMaximumValues.SetProperty("staminaRate",
                              JsValue::Int(maximumValues.staminaRate));

  jsMaximumValues.SetProperty("healRateMult",
                              JsValue::Int(maximumValues.healRateMult));

  jsMaximumValues.SetProperty("magickaRateMult",
                              JsValue::Int(maximumValues.magickaRateMult));

  jsMaximumValues.SetProperty("staminaRateMult",
                              JsValue::Int(maximumValues.staminaRateMult));

  return jsMaximumValues;
}

JsValue GetRaceId(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  return JsValue::Int(actor.GetRaceId());
}

JsValue IsDead(std::shared_ptr<PartOne> partOne,
               const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  return JsValue::Bool(actor.IsDead());
}

JsValue IsRaceMenuOpen(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  return JsValue::Bool(actor.IsRaceMenuOpen());
}

JsValue IsRespawning(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  return JsValue::Bool(actor.IsRespawning());
}

JsValue IsWeaponDrawn(std::shared_ptr<PartOne> partOne,
                      const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  return JsValue::Bool(actor.IsWeaponDrawn());
}

JsValue Kill(std::shared_ptr<PartOne> partOne, const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  auto killerFormId = Uint32FromJsValue(args[1].GetProperty("_formId"));

  if (killerFormId != 0) {
    auto& killer = partOne->worldState.GetFormAt<MpActor>(killerFormId);
    bool shouldTeleport = args[2];

    if (shouldTeleport) {
      actor.Kill(&killer, shouldTeleport);
      return JsValue::Undefined();
    }

    actor.Kill(&killer);
    return JsValue::Undefined();
  }

  actor.Kill();

  return JsValue::Undefined();
}

JsValue Respawn(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  bool shouldTeleport = args[1];

  if (shouldTeleport) {
    actor.Respawn(shouldTeleport);
    return JsValue::Undefined();
  }

  actor.Respawn();

  return JsValue::Undefined();
}

JsValue RestoreActorValue(std::shared_ptr<PartOne> partOne,
                          const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  auto av = static_cast<espm::ActorValue>(Uint32FromJsValue(args[1]));
  auto value = static_cast<double>(args[2]);

  actor.RestoreActorValue(av, value);

  return JsValue::Undefined();
}

JsValue SetActorValuesPercentages(std::shared_ptr<PartOne> partOne,
                                  const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  auto health = static_cast<double>(args[1]);
  auto magicka = static_cast<double>(args[2]);
  auto stamina = static_cast<double>(args[3]);

  auto aggressorId = Uint32FromJsValue(args[4].GetProperty("_formId"));

  if (aggressorId == 0) {
    auto& aggressor = partOne->worldState.GetFormAt<MpActor>(formId);

    actor.SetPercentages(health, magicka, stamina, &aggressor);

    return JsValue::Undefined();
  }

  actor.SetPercentages(health, magicka, stamina);

  return JsValue::Undefined();
}

JsValue SetRaceMenuOpen(std::shared_ptr<PartOne> partOne,
                        const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  bool isOpen = args[1];

  actor.SetRaceMenuOpen(isOpen);

  return JsValue::Undefined();
}

JsValue SetSpawnPoint(std::shared_ptr<PartOne> partOne,
                      const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  JsValue jsLocationalData = args[1];
  auto pos = jsLocationalData.GetProperty("pos");
  auto rot = jsLocationalData.GetProperty("rot");

  LocationalData locationalData = {
    {
      FloatFromJsValue(pos.GetProperty(0)),
      FloatFromJsValue(pos.GetProperty(1)),
      FloatFromJsValue(pos.GetProperty(2)),
    },
    {
      FloatFromJsValue(rot.GetProperty(0)),
      FloatFromJsValue(rot.GetProperty(1)),
      FloatFromJsValue(rot.GetProperty(2)),
    },
    FormDesc::FromString(jsLocationalData.GetProperty("cellOrWorldDesc"))
  };

  actor.SetSpawnPoint(locationalData);

  return JsValue::Undefined();
}

JsValue GetSpawnPoint(std::shared_ptr<PartOne> partOne,
                      const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  LocationalData locationalData = actor.GetSpawnPoint();

  JsValue jsLocationalData = JsValue::Object();

  JsValue jsPos = JsValue::Array(3);
  jsPos.SetProperty(0, JsValue(locationalData.pos[0]));
  jsPos.SetProperty(0, JsValue(locationalData.pos[1]));
  jsPos.SetProperty(0, JsValue(locationalData.pos[2]));
  jsLocationalData.SetProperty("pos", jsPos);

  JsValue jsRot = JsValue::Array(3);
  jsRot.SetProperty(0, JsValue(locationalData.rot[0]));
  jsRot.SetProperty(0, JsValue(locationalData.rot[1]));
  jsRot.SetProperty(0, JsValue(locationalData.rot[2]));
  jsLocationalData.SetProperty("rot", jsRot);

  JsValue cellOrWorldDesc = locationalData.cellOrWorldDesc.ToString();
  jsLocationalData.SetProperty("cellOrWorldDesc", cellOrWorldDesc);

  return jsLocationalData;
}

JsValue GetRespawnTime(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  auto respawnTime = actor.GetRespawnTime();

  return JsValue(respawnTime);
}

JsValue SetRespawnTime(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);

  auto respawnTime = args[1];

  actor.SetRespawnTime(static_cast<double>(respawnTime));

  return JsValue::Undefined();
}
