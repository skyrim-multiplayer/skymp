#include "PapyrusActor.h"

#include "EspmGameObject.h"
#include "Inventory.h"
#include "MpActor.h"
#include "MpForm.h"
#include "MpFormGameObject.h"
#include "MsgType.h"

#include "CIString.h"

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
    auto& form = GetRecordPtr(arguments[0]);
    if (!form.rec) {
      throw std::runtime_error(
        fmt::format("Unable to find a record of a form"));
    }
    if (!actor->GetParent()->HasEspm()) {
      throw std::runtime_error(fmt::format(
        "No espm attached to the actor with id {:x}", actor->GetBaseId()));
    }
    auto& loader = actor->GetParent()->GetEspm();
    auto& cache = actor->GetParent()->GetEspmCache();

    const Equipment eq = actor->GetEquipment();

    actor->OnEquip(form.rec->GetId());
    Equipment newEq;
    newEq.numChanges = eq.numChanges + 1;
    for (auto& entry : eq.inv.entries) {
      bool isEquipped = entry.extra.worn != Inventory::Worn::None;
      bool isWeap = espm::GetRecordType(entry.baseId, actor->GetParent()) ==
        espm::WEAP::kType;
      if (entry.baseId == form.rec->GetId()) {
        if (isWeap) {
          continue;
        }
        Inventory::Entry modifiedEntry = entry;
        modifiedEntry.extra.worn = Inventory::Worn::Right;
        newEq.inv.AddItems({ modifiedEntry });
        continue;
      }
      if (isEquipped && isWeap) {
        continue;
      }
      newEq.inv.AddItems({ entry });
    }
    const Inventory inv = actor->GetInventory();
    Inventory::Entry bestEntry;
    int16_t bestDamage = -1;
    for (auto& entry : inv.entries) {
      if (entry.baseId) {
        auto lookupRes = loader.GetBrowser().LookupById(entry.baseId);
        if (auto weap = espm::Convert<espm::WEAP>(lookupRes.rec)) {
          if (!bestEntry.count ||
              weap->GetData(cache).weapData->damage > bestDamage) {
            bestEntry = entry;
            bestDamage = weap->GetData(cache).weapData->damage;
          }
        }
      }
    }
    if (bestEntry.count > 0) {
      bestEntry.extra.worn = Inventory::Worn::Right;
      newEq.inv.AddItems({ bestEntry });
    }
    actor->SetEquipment(newEq.ToJson().dump());
    for (auto listener : actor->GetListeners()) {
      auto actor = dynamic_cast<MpActor*>(listener);
      if (!actor) {
        continue;
      }
      std::string s;
      s += Networking::MinPacketId;
      s += nlohmann::json{
        { "t", MsgType::UpdateEquipment },
        { "idx", actor->GetIdx() },
        { "data", newEq.ToJson() }
      }.dump();
      actor->SendToUser(s.data(), s.size(), true);
    }
  }
  return VarValue::None();
}
