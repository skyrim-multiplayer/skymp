#include "SweetPieScript.h"

#include "FormDesc.h"
#include "MpActor.h"
#include "NiPoint3.h"
#include "SpSnippet.h"
#include "SweetPieBoundWeapon.h"
#include "TimeUtils.h"
#include "WorldState.h"
#include "libespm/espm.h"
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

std::mt19937 g_rng{ std::random_device{}() };

uint32_t GenerateRandomNumber(uint32_t leftBound, uint32_t rightBound)
{
  if (leftBound <= rightBound) {
    std::uniform_int_distribution<> distr(leftBound, rightBound);
    return distr(g_rng);
  } else {
    throw std::runtime_error(fmt::format(
      "GenerateRandomNumber() cannot generate number in range: ({}, {})",
      leftBound, rightBound));
  }
}

SweetPieScript::SweetPieScript(const std::vector<std::string>& espmFiles)
{
  miscLootTable = { { 0x07A45089, { 0x07A30B91 } },
                    { 0x07A4508b, { 0x07A4A191, 0x0010B0A7 } },
                    { 0x07A4A18F, { 0x07A30B93 } },
                    { 0x07A4A18D, { 0x07A30B92 } },
                    { 0x07ABE9F6, { 0x07A59508, 0x07A59509 } },
                    { 0x07ABE9F8, { 0x07A59506, 0x07A59507 } },
                    { 0x07ABE9FE, { 0x07A5950C, 0x07A5950D } },
                    { 0x07ABEA00, { 0x07A5950F, 0x07A59510 } },
                    { 0x07ABE9FA, { 0x07A59504, 0x07A59505 } },
                    { 0x07ABE9FC, { 0x07A5950A, 0x07A5950B } },
                    { 0x071746c7, { 0x070da797 } },
                    { 0x071746d1, { 0x070da799 } },
                    { 0x071746c3, { 0x070da796 } },
                    { 0x071746bf, { 0x07a5950e } },
                    { 0x071746c1, { 0x070da795 } },
                    { 0x071746c9, { 0x07fa85da } },
                    { 0x071746cf, { 0x07fa85d8 } },
                    { 0x071746cd, { 0x07fa85de } },
                    { 0x071746cb, { 0x07fa85df } },
                    { 0x07183a13, { 0x07b84646 } },
                    { 0x071746be, { 0x070df89c } },
                    { 0x07267fe5, { 0x07267fd9 } },
                    { 0x07267fe3, { 0x07267fd8 } },
                    { 0x07267fe1, { 0x0726f5a8 } },
                    { 0x07267fe9, { 0x07267fdc } },
                    { 0x07267fe7, { 0x07267fdb } },
                    { 0x07267fef, { 0x07267fda } },
                    { 0x07267fde, { 0x07267fdd } } };

  bookBoundWeapons = {
    { 0x0401ce07, { 0x07f42cb6, SweetPieBoundWeapon::SkillLevel::Novice } },
    { 0x07f42cc2, { 0x7a30b931, SweetPieBoundWeapon::SkillLevel::Novice } },
    { 0x07f5c2ad, { 0x07f42cb5, SweetPieBoundWeapon::SkillLevel::Adept } },
    { 0x000a26f1, { 0x07a4a191, SweetPieBoundWeapon::SkillLevel::Adept } },
    { 0x07f42cc1, { 0x07f42cb4, SweetPieBoundWeapon::SkillLevel::Expert } },
    { 0x000a26ed, { 0x00058f5e, SweetPieBoundWeapon::SkillLevel::Expert } },
    { 0x07f38aab, { 0x07f42caf, SweetPieBoundWeapon::SkillLevel::Master } },
    { 0x0009e2a9, { 0x00058f5f, SweetPieBoundWeapon::SkillLevel::Master } },
  };
}

void SweetPieScript::AddItem(MpActor& actor, const WorldState& worldState,
                             uint32_t itemBaseId, uint32_t count)
{
  actor.AddItem(itemBaseId, count);
  Notify(actor, worldState, itemBaseId, count, false);
}

void SweetPieScript::Notify(MpActor& actor, const WorldState& worldState,
                            uint32_t formId, uint32_t count, bool silent)
{
  std::string type;
  std::stringstream ss;
  auto lookupRes = worldState.GetEspm().GetBrowser().LookupById(formId);
  auto recType = lookupRes.rec->GetType();

  if (recType == "WEAP") {
    type = "weapon";
  } else if (recType == "ARMO") {
    type = "armor";
  } else if (recType == "INGR") {
    type = "ingridient";
  } else if (recType == "LIGH") {
    type = "light";
  } else if (recType == "SLGM") {
    type = "soulgem";
  } else if (recType == "ALCH") {
    type = "potion";
  } else {
    throw std::runtime_error(fmt::format("Unexpected type {}", type));
  }

  ss << "[";
  ss << nlohmann::json({ { "formId", formId }, { "type", type } }).dump();
  ss << "," << static_cast<uint32_t>(count) << ","
     << (static_cast<bool>(silent) ? "true" : "false");
  ss << "]";
  std::string args = ss.str();
  (void)SpSnippet("SkympHacks", "AddItem", args.data()).Execute(&actor);
}

void SweetPieScript::Play(MpActor& actor, WorldState& worldState,
                          uint32_t itemBaseId)
{
  bool isWardrobePie = itemBaseId == EdibleItems::kWardrobePie;
  if (isWardrobePie) {
    constexpr uint32_t wardrobeId = 0x0756C165;
    const NiPoint3 wardrobePos = { -769, 10461, -915 };
    actor.Teleport({ wardrobePos,
                     { 0, 0, 0 },
                     FormDesc::FromFormId(wardrobeId, worldState.espmFiles) });
  }

  if (auto it = miscLootTable.find(itemBaseId); it != miscLootTable.end()) {
    for (const auto& item : miscLootTable[itemBaseId]) {
      AddItem(actor, worldState, item, 1);
    }
  }

  if (auto it = bookBoundWeapons.find(itemBaseId);
      it != bookBoundWeapons.end()) {
    float currentMagickaPercentage =
      actor.GetChangeForm().actorValues.magickaPercentage;
    if (currentMagickaPercentage >= it->second.GetManacostPercentage()) {
      actor.DamageActorValue(espm::ActorValue::Magicka,
                             it->second.GetManacost());
      uint32_t boundWeaponBaseId = it->second.GetBaseId(),
               bookBaseId = it->first;
      actor.AddItem(boundWeaponBaseId, 1);
      EquipItem(actor, boundWeaponBaseId);
      actor.RemoveItem(bookBaseId, 1, nullptr);
      uint32_t formId = actor.GetFormId();
      float cooldown = it->second.GetCooldown();
      worldState
        .SetTimer(Viet::TimeUtils::To<std::chrono::milliseconds>(cooldown))
        .Then(
          [&worldState, bookBaseId, boundWeaponBaseId, formId](Viet::Void) {
            MpActor& actor = worldState.GetFormAt<MpActor>(formId);
            actor.AddItem(bookBaseId, 1);
            uint32_t count =
              actor.GetInventory().GetItemCount(boundWeaponBaseId);
            actor.RemoveItem(boundWeaponBaseId, count, nullptr);
          });
    }
  }
}

void SweetPieScript::EquipItem(MpActor& actor, uint32_t baseId,
                               bool preventRemoval, bool silent)
{
  std::stringstream ss;
  ss << "["
     << nlohmann::json{ { "formId", baseId }, { "type", "weapon" } }.dump()
     << ", " << (preventRemoval ? "true" : "false") << ", "
     << (silent ? "true" : "false") << "]";
  std::string args = ss.str();
  spdlog::info(args);
  SpSnippet("Actor", "EquipItem", args.data(), actor.GetFormId())
    .Execute(&actor);
  SpSnippet("Actor", "DrawWeapon", "[]", actor.GetFormId()).Execute(&actor);
}
