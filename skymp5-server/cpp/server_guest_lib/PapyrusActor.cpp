#include "PapyrusActor.h"

#include "MpActor.h"
#include "MpFormGameObject.h"

#include "SpSnippetFunctionGen.h"
#include "papyrus-vm/CIString.h"
#include <algorithm>

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
  float modifier = static_cast<double>(arguments[1]);
  if (auto actor = GetFormPtr<MpActor>(self)) {
    actor->RestoreActorValue(attributeName, modifier);
  }
  return VarValue();
}

VarValue PapyrusActor::SetActorValue(VarValue self,
                                     const std::vector<VarValue>& arguments)
{
  if (auto actor = GetFormPtr<MpActor>(self)) {

    // TODO: fix that at least for important AVs like attributes
    // SpSnippet impl helps scripted draugrs attack, nothing more (Aggression
    // var)
    spdlog::warn("SetActorValue executes locally at this moment. Results will "
                 "not affect server calculations");

    auto it = actor->GetParent()->hosters.find(actor->GetFormId());

    auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);

    // spsnippet don't support auto sending to host. so determining current
    // hoster explicitly
    SpSnippet(GetName(), "SetActorValue", serializedArgs.data(),
              actor->GetFormId())
      .Execute(it == actor->GetParent()->hosters.end()
                 ? actor
                 : &actor->GetParent()->GetFormAt<MpActor>(it->second));
  }
  return VarValue();
}

VarValue PapyrusActor::DamageActorValue(VarValue self,
                                        const std::vector<VarValue>& arguments)
{
  espm::ActorValue attributeName =
    ConvertToAV(static_cast<const char*>(arguments[0]));
  float modifier = static_cast<double>(arguments[1]);
  if (auto actor = GetFormPtr<MpActor>(self)) {
    actor->DamageActorValue(attributeName, modifier);
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

VarValue PapyrusActor::IsDead(
  VarValue self, const std::vector<VarValue>& arguments) const noexcept
{
  if (auto _this = GetFormPtr<MpActor>(self)) {
    return VarValue(_this->IsDead());
  }
  return VarValue::None();
}

VarValue PapyrusActor::WornHasKeyword(VarValue self,
                                      const std::vector<VarValue>& arguments)
{
  if (auto actor = GetFormPtr<MpActor>(self)) {
    if (arguments.size() < 1) {
      throw std::runtime_error(
        "Actor.WornHasKeyword requires at least one argument");
    }

    const auto& keywordRec = GetRecordPtr(arguments[0]);
    if (!keywordRec.rec) {
      spdlog::error("Actor.WornHasKeyword - invalid keyword form");
      return VarValue(false);
    }

    const std::vector<Inventory::Entry>& entries =
      actor->GetEquipment().inv.entries;
    WorldState* worldState = compatibilityPolicy->GetWorldState();
    for (const auto& entry : entries) {
      if (entry.extra.worn != Inventory::Worn::None) {
        const espm::LookupResult res =
          worldState->GetEspm().GetBrowser().LookupById(entry.baseId);
        if (!res.rec) {
          return VarValue::None();
        }
        const auto keywordIds =
          res.rec->GetKeywordIds(worldState->GetEspmCache());
        if (std::any_of(keywordIds.begin(), keywordIds.end(),
                        [&](uint32_t keywordId) {
                          return res.ToGlobalId(keywordId) ==
                            keywordRec.ToGlobalId(keywordRec.rec->GetId());
                        })) {
          return VarValue(true);
        }
      }
    }
  }
  return VarValue(false);
}

void PapyrusActor::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddMethod(vm, "IsWeaponDrawn", &PapyrusActor::IsWeaponDrawn);
  AddMethod(vm, "DrawWeapon", &PapyrusActor::DrawWeapon);
  AddMethod(vm, "UnequipAll", &PapyrusActor::UnequipAll);
  AddMethod(vm, "PlayIdle", &PapyrusActor::PlayIdle);
  AddMethod(vm, "GetSitState", &PapyrusActor::GetSitState);
  AddMethod(vm, "RestoreActorValue", &PapyrusActor::RestoreActorValue);
  AddMethod(vm, "SetActorValue", &PapyrusActor::SetActorValue);
  AddMethod(vm, "DamageActorValue", &PapyrusActor::DamageActorValue);
  AddMethod(vm, "IsEquipped", &PapyrusActor::IsEquipped);
  AddMethod(vm, "GetActorValuePercentage",
            &PapyrusActor::GetActorValuePercentage);
  AddMethod(vm, "SetAlpha", &PapyrusActor::SetAlpha);
  AddMethod(vm, "EquipItem", &PapyrusActor::EquipItem);
  AddMethod(vm, "SetDontMove", &PapyrusActor::SetDontMove);
  AddMethod(vm, "IsDead", &PapyrusActor::IsDead);
}
