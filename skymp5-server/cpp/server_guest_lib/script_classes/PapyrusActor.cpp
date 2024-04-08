#include "PapyrusActor.h"

#include "MpActor.h"
#include "script_objects/EspmGameObject.h"
#include "script_objects/MpFormGameObject.h"

#include "EvaluateTemplate.h"
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
                 : &actor->GetParent()->GetFormAt<MpActor>(it->second),
               SpSnippetMode::kNoReturnResult);
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
          .Execute(targetRefr, SpSnippetMode::kNoReturnResult);
      }
    }
  }
  return VarValue::None();
}

VarValue PapyrusActor::EquipItem(VarValue self,
                                 const std::vector<VarValue>& arguments)
{
  if (auto actor = GetFormPtr<MpActor>(self)) {
    auto worldState = actor->GetParent();
    if (!worldState) {
      throw std::runtime_error("EquipItem - no WorldState attached");
    }

    if (arguments.size() < 1) {
      throw std::runtime_error("EquipItem requires at least one argument");
    }

    auto lookupRes = GetRecordPtr(arguments[0]);
    if (!lookupRes.rec) {
      throw std::runtime_error("EquipItem - invalid form");
    }

    if (!espm::utils::IsItem(lookupRes.rec->GetType())) {
      throw std::runtime_error("EquipItem - form is not an item");
    }

    if (espm::utils::Is<espm::LIGH>(lookupRes.rec->GetType())) {
      auto res = espm::Convert<espm::LIGH>(lookupRes.rec)
                   ->GetData(worldState->GetEspmCache());
      bool isTorch = res.data.flags & espm::LIGH::Flags::CanBeCarried;
      if (!isTorch) {
        throw std::runtime_error(
          "EquipItem - form is LIGH without CanBeCarried flag");
      }
    }

    // If no such item in inventory, add one (this is standard behavior)
    auto baseId = lookupRes.ToGlobalId(lookupRes.rec->GetId());
    if (actor->GetInventory().GetItemCount(baseId) == 0) {
      actor->AddItem(baseId, 1);
    }

    SpSnippet(GetName(), "EquipItem",
              SpSnippetFunctionGen::SerializeArguments(arguments).data(),
              actor->GetFormId())
      .Execute(actor, SpSnippetMode::kNoReturnResult);
  }
  return VarValue::None();
}

VarValue PapyrusActor::UnequipItem(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 3) {
    throw std::runtime_error("UnequipItem requires at least 3 arguments");
  }

  if (auto actor = GetFormPtr<MpActor>(self)) {
    SpSnippet(GetName(), "UnequipItem",
              SpSnippetFunctionGen::SerializeArguments(arguments).data(),
              actor->GetFormId())
      .Execute(actor, SpSnippetMode::kNoReturnResult);
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
      .Execute(actor, SpSnippetMode::kNoReturnResult);
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

VarValue PapyrusActor::AddSpell(VarValue self,
                                const std::vector<VarValue>& arguments)
{
  // TODO: should we sync spell list in general? should we show spell add for
  // actor neighbors?

  if (auto actor = GetFormPtr<MpActor>(self)) {
    if (arguments.size() < 2) {
      throw std::runtime_error(
        "Actor.AddSpell requires at least two arguments");
    }

    const auto& spell = GetRecordPtr(arguments[0]);
    if (!spell.rec) {
      spdlog::error("Actor.AddSpell - invalid spell form");
      return VarValue(false);
    }

    if (spell.rec->GetType().ToString() != "SPEL") {
      spdlog::error("Actor.AddSpell - type expected to be SPEL, but it is {}",
                    spell.rec->GetType().ToString());
      return VarValue(false);
    }

    uint32_t spellId = spell.ToGlobalId(spell.rec->GetId());

    if (!actor->IsSpellLearned(spellId)) {
      actor->AddSpell(spellId);

      SpSnippet(GetName(), "AddSpell",
                SpSnippetFunctionGen::SerializeArguments(arguments).data(),
                actor->GetFormId())
        .Execute(actor, SpSnippetMode::kNoReturnResult);

      return VarValue(true);
    }
  }

  return VarValue(false);
}

VarValue PapyrusActor::RemoveSpell(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  if (auto actor = GetFormPtr<MpActor>(self)) {
    if (arguments.size() < 1) {
      throw std::runtime_error(
        "Actor.RemoveSpell requires at least one argument");
    }

    const auto& spell = GetRecordPtr(arguments[0]);
    if (!spell.rec) {
      spdlog::error("Actor.RemoveSpell - invalid spell form");
      return VarValue(false);
    }

    if (spell.rec->GetType().ToString() != "SPEL") {
      spdlog::error(
        "Actor.RemoveSpell - type expected to be SPEL, but it is {}",
        spell.rec->GetType().ToString());
      return VarValue(false);
    }

    uint32_t spellId = spell.ToGlobalId(spell.rec->GetId());

    if (actor->IsSpellLearnedFromBase(spellId)) {
      spdlog::warn("Actor.RemoveSpell - can't remove spells inherited from "
                   "RACE/NPC_ records");
    } else if (!actor->IsSpellLearned(spellId)) {
      spdlog::warn("Actor.RemoveSpell - spell already removed/not learned");
    } else {
      actor->RemoveSpell(spellId);

      SpSnippet(GetName(), "RemoveSpell",
                SpSnippetFunctionGen::SerializeArguments(arguments).data(),
                actor->GetFormId())
        .Execute(actor, SpSnippetMode::kNoReturnResult);

      return VarValue(true);
    }
  }
  return VarValue(false);
}

VarValue PapyrusActor::GetRace(VarValue self,
                               const std::vector<VarValue>& arguments)
{
  auto actor = GetFormPtr<MpActor>(self);
  if (!actor) {
    return VarValue::None();
  }

  uint32_t raceId = 0;

  if (auto appearance = actor->GetAppearance()) {
    raceId = appearance->raceId;
  } else {
    raceId = EvaluateTemplate<espm::NPC_::UseTraits>(
      actor->GetParent(), actor->GetBaseId(), actor->GetTemplateChain(),
      [](const auto& npcLookupResult, const auto& npcData) {
        return npcLookupResult.ToGlobalId(npcData.race);
      });
  }

  auto lookupRes =
    actor->GetParent()->GetEspm().GetBrowser().LookupById(raceId);

  if (!lookupRes.rec) {
    spdlog::error("Actor.GetRace - Race with id {:x} not found in espm",
                  raceId);
    return VarValue::None();
  }

  if (!(lookupRes.rec->GetType() == espm::RACE::kType)) {
    spdlog::error(
      "Actor.GetRace - Expected record {:x} to be RACE, but it is {}", raceId,
      lookupRes.rec->GetType().ToString());
    return VarValue::None();
  }

  return VarValue(std::make_shared<EspmGameObject>(lookupRes));
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
  AddMethod(vm, "UnequipItem", &PapyrusActor::UnequipItem);
  AddMethod(vm, "SetDontMove", &PapyrusActor::SetDontMove);
  AddMethod(vm, "IsDead", &PapyrusActor::IsDead);
  AddMethod(vm, "WornHasKeyword", &PapyrusActor::WornHasKeyword);
  AddMethod(vm, "AddSpell", &PapyrusActor::AddSpell);
  AddMethod(vm, "RemoveSpell", &PapyrusActor::RemoveSpell);
  AddMethod(vm, "GetRace", &PapyrusActor::GetRace);
}
