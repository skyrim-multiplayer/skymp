#include "PapyrusActor.h"

#include "MpActor.h"
#include "MpFormGameObject.h"

#include "SpSnippetFunctionGen.h"
#include "papyrus-vm/CIString.h"

namespace {
espm::ActorValue ConvertToAV(CIString actorValueName)
{
  if (!actorValueName.compare("health")) {
    return espm::ActorValue::Health;
  }
  if (!actorValueName.compare("stamina")) {
    return espm::ActorValue::Stamina;
  }
  if (!actorValueName.compare("magicka")) {
    return espm::ActorValue::Magicka;
  }
  return espm::ActorValue::None;
}
}

VarValue PapyrusActor::IsWeaponDrawn(VarValue self,
                                     const std::vector<VarValue>& arguments)
{
  if (auto actor = GetFormPtr<MpActor>(self)) {
    return VarValue(actor->IsWeaponDrawn());
  }
  return VarValue(false);
}

VarValue PapyrusActor::RestoreActorValue(
  VarValue self, const std::vector<VarValue>& arguments)
{
  espm::ActorValue attributeName =
    ConvertToAV(static_cast<const char*>(arguments[0]));
  float modifire = static_cast<double>(arguments[1]);
  if (auto actor = GetFormPtr<MpActor>(self)) {
    actor->RestoreActorValue(attributeName, modifire);
  }
  return VarValue();
}

VarValue PapyrusActor::DamageActorValue(VarValue self,
                                        const std::vector<VarValue>& arguments)
{
  espm::ActorValue attributeName =
    ConvertToAV(static_cast<const char*>(arguments[0]));
  float modifire = static_cast<double>(arguments[1]);
  if (auto actor = GetFormPtr<MpActor>(self)) {
    actor->DamageActorValue(attributeName, modifire);
  }
  return VarValue();
}

VarValue PapyrusActor::IsEquipped(VarValue self,
                                  const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 1) {
    throw std::runtime_error("Papyrus Actor IsEquipped: wrong argument count");
  }

  auto selfRefr = GetFormPtr<MpForm>(self);
  auto actor = GetFormPtr<MpActor>(self);
  auto form = GetRecordPtr(arguments[0]);

  if (!form.rec) {
    return VarValue(false);
  }

  std::vector<uint32_t> formIds;

  if (auto formlist = espm::Convert<espm::FLST>(form.rec)) {
    formIds =
      espm::GetData<espm::FLST>(formlist->GetId(), selfRefr->GetParent())
        .formIds;
  } else {
    formIds.emplace_back(form.ToGlobalId(form.rec->GetId()));
  }

  auto equipment = actor->GetEquipment().inv;
  // Enum entries of equipment
  for (auto& entry : equipment.entries) {
    // Filter out non-worn (in current implementation it is possible)
    if (entry.extra.worn == Inventory::Worn::Right ||
        entry.extra.worn == Inventory::Worn::Left) {
      // Enum entries of form list
      for (const auto& formId : formIds) {
        // If one of equipment entries matches one of formlist entries, then
        // return true
        if (entry.baseId == formId) {
          return VarValue(true);
        }
      }
    }
  }

  return VarValue(false);
}

VarValue PapyrusActor::GetActorValuePercentage(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 1) {
    throw std::runtime_error(
      "Papyrus Actor.GetActorValuePercentage: wrong argument count");
  }

  if (auto actor = GetFormPtr<MpActor>(self)) {
    espm::ActorValue attrID =
      ConvertToAV(static_cast<const char*>(arguments[0]));

    auto form = actor->GetChangeForm();
    if (attrID == espm::ActorValue::Health) {
      return VarValue(form.actorValues.healthPercentage);
    } else if (attrID == espm::ActorValue::Stamina) {
      return VarValue(form.actorValues.staminaPercentage);
    } else if (attrID == espm::ActorValue::Magicka) {
      return VarValue(form.actorValues.magickaPercentage);
    } else {
      return VarValue(0.0f);
    }
  }
  return VarValue(0.0f);
}

VarValue PapyrusActor::SetAlpha(VarValue self,
                                const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpActor>(self)) {
    if (arguments.size() < 1) {
      throw std::runtime_error("SetAlpha requires at least one argument");
    }
    // TODO: Make normal sync for this. For now using workaround to inform
    // neigbours by sending papyrus functions to them.
    auto funcName = "SetAlpha";
    auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
    for (auto listener : selfRefr->GetListeners()) {
      auto targetRefr = dynamic_cast<MpActor*>(listener);
      if (targetRefr) {
        SpSnippet(GetName(), funcName, serializedArgs.data(),
                  selfRefr->GetFormId())
          .Execute(targetRefr);
      }
    }
  }
  return VarValue::None();
}

VarValue PapyrusActor::EquipItem(VarValue self,
                                 const std::vector<VarValue>& arguments)
{
  if (auto actor = GetFormPtr<MpActor>(self)) {
    if (arguments.size() < 1) {
      throw std::runtime_error("EquipItem requires at least one argument");
    }
    SpSnippet(GetName(), "EquipItem",
              SpSnippetFunctionGen::SerializeArguments(arguments).data(),
              actor->GetFormId())
      .Execute(actor);
  }
  return VarValue::None();
}

VarValue PapyrusActor::SetDontMove(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  if (auto actor = GetFormPtr<MpActor>(self)) {
    if (arguments.size() < 1) {
      throw std::runtime_error("SetDontMove requires at least one argument");
    }
    SpSnippet(GetName(), "SetDontMove",
              SpSnippetFunctionGen::SerializeArguments(arguments).data(),
              actor->GetFormId())
      .Execute(actor);
  }
  return VarValue::None();
}
