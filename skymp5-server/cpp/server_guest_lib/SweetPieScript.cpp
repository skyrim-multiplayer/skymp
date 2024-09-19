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
#include <spdlog/spdlog.h>
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
  // TODO: check other ids
  miscLootTable = {
    { 0x07A45089, { 0x07A30B91 } },
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
    { 0x071746cf, { 0x07fa85db } },
    { 0x071746cd, { 0x07fa85de } },
    { 0x071746cb, { 0x07fa85df } },
    { 0x07183a13, { 0x07b84646 } },
    { 0x071746be, { 0x070df89c } },
    { 0x07267fe5, { 0x07267fd9 } },
    { 0x07267fe3, { 0x07267fd8 } },

    { 0x07267fe1, { 0x0716f5a8 } },

    { 0x07267fe9, { 0x07267fdc } },
    { 0x07267fe7, { 0x07267fdb } },
    { 0x07267fef, { 0x07267fda } },
    { 0x07267fde, { 0x07267fdd } },
    { 0x0720f102, { 0x0720f101 } },
    { 0x0720f103, { 0x0720f100 } },
    { 0x0720f105, { 0x0720f0ff } },
    { 0x0720f107, { 0x07019da3 } },
    { 0x070d3b6a, { 0x0001d4ec } },
    { 0x0720f10d, { 0x007b8484 } },
    { 0x0720f110, { 0x0720f10e } },
    { 0x071746c5, { 0x070da798 } },
    { 0x07000823, { 0x07000808 } },
    { 0x07000824, { 0x07005905, 0x07005906 } },
    { 0x07005908, { 0x07005904 } },
    { 0x070A696C, { 0x070782C9 } },
    { 0x070A696D, { 0x070782C8 } },
    { 0x07d6e896, { 0x07d6e890 } },
    { 0x07d6e897, { 0x07d6e891 } },
    { 0x07d6e899, { 0x07d6e892 } },
    { 0x07d6e89b, { 0x07d6e893 } },
    { 0x07d6e89d, { 0x07d6e894 } },
    { 0x07d6e89f, { 0x07d6e88f } },
    { 0x07d6e8a1, { 0x07d6e895 } },
    { 0x07d6e8ab, { 0x07d6e8ac } },
  };

  bookBoundWeapons = {
    { 0x0401ce07,
      { { 0x07f42cb6, SweetPieBoundWeapon::SkillLevel::Novice } } },
    { 0x07f42cc2,
      { { 0x7a30b931, SweetPieBoundWeapon::SkillLevel::Novice } } },
    { 0x07f5c2ad, { { 0x07f42cb5, SweetPieBoundWeapon::SkillLevel::Adept } } },
    { 0x000a26f1, { { 0x07a4a191, SweetPieBoundWeapon::SkillLevel::Adept } } },
    { 0x07f42cc1,
      { { 0x07f42cb4, SweetPieBoundWeapon::SkillLevel::Expert } } },
    { 0x000a26ed,
      { { 0x00058f5e, SweetPieBoundWeapon::SkillLevel::Expert } } },
    { 0x07f38aab,
      { { 0x07f42caf, SweetPieBoundWeapon::SkillLevel::Master } } },
    { 0x0009e2a9,
      { { 0x00058f5f, SweetPieBoundWeapon::SkillLevel::Master } } },
    { 0x7276E2A, { { 0x7A30B91, SweetPieBoundWeapon::SkillLevel::Novice } } },
    { 0x7276E39,
      { { 0x7A4A191, SweetPieBoundWeapon::SkillLevel::Novice },
        { 0x010B0A7, SweetPieBoundWeapon::SkillLevel::Novice } } },
    { 0x7276E3B, { { 0x7A30B93, SweetPieBoundWeapon::SkillLevel::Novice } } },
    { 0x7276E37, { { 0x7F42CAF, SweetPieBoundWeapon::SkillLevel::Novice } } },
    { 0x7276E35, { { 0x7F42CB5, SweetPieBoundWeapon::SkillLevel::Novice } } },
    { 0x7276E33, { { 0x7F42CB4, SweetPieBoundWeapon::SkillLevel::Novice } } },
    { 0x7276E2d, { { 0x7F42CB6, SweetPieBoundWeapon::SkillLevel::Novice } } },
    { 0x7276E2f,
      { { 0x7A30B92, SweetPieBoundWeapon::SkillLevel::Novice },
        { 0x7F47DC9, SweetPieBoundWeapon::SkillLevel::Novice } } },
    { 0x72BDE4A, { { 0x72B8D47, SweetPieBoundWeapon::SkillLevel::Novice } } },
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

  if (!lookupRes.rec) {
    return spdlog::error(
      "SweetPieScript::Notify - formId {:x} not found in espm");
  }

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
    return spdlog::error(
      "SweetPieScript::Notify - Unexpected type {} in formId {:x}", type,
      formId);
  }

  ss << "[";
  ss << nlohmann::json({ { "formId", formId }, { "type", type } }).dump();
  ss << "," << static_cast<uint32_t>(count) << ","
     << (static_cast<bool>(silent) ? "true" : "false");
  ss << "]";
  std::string args = ss.str();
  (void)SpSnippet("SkympHacks", "AddItem", args.data())
    .Execute(&actor, SpSnippetMode::kNoReturnResult);
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
    for (const auto& boundItem : it->second) {
      bool isArrow = boundItem.GetBaseId() == 0x10B0A7;
      if (currentMagickaPercentage >= boundItem.GetManacostPercentage()) {
        if (!isArrow) {
          actor.DamageActorValue(espm::ActorValue::Magicka,
                                 boundItem.GetManacost());
        }
        uint32_t boundWeaponBaseId = boundItem.GetBaseId(),
                 bookBaseId = it->first;
        actor.AddItem(boundWeaponBaseId, isArrow ? 40 : 1);
        EquipItem(actor, boundWeaponBaseId);
        uint32_t formId = actor.GetFormId();
        float cooldown = boundItem.GetCooldown();
        auto endTime =
          Viet::TimeUtils::To<std::chrono::milliseconds>(cooldown);
        worldState.SetTimer(endTime).Then(
          [&worldState, bookBaseId, boundWeaponBaseId, formId](Viet::Void) {
            MpActor& actor = worldState.GetFormAt<MpActor>(formId);
            // removing book before adding it to a player, because there exists
            // books that add more than one item. In this case we'll be adding
            // the same book n times. This is a workaround. TODO: refactor this
            // class somewhen
            if (actor.GetInventory().HasItem(bookBaseId)) {
              actor.RemoveItem(bookBaseId,
                               actor.GetInventory().GetItemCount(bookBaseId),
                               nullptr);
            }
            actor.AddItem(bookBaseId, 1);
            uint32_t count =
              actor.GetInventory().GetItemCount(boundWeaponBaseId);
            actor.RemoveItem(boundWeaponBaseId, count, nullptr);
          });
      } else {
        // adding book back if there is insufficient mana. Perhaps, this case
        // should be validated on the client side
        actor.AddItem(it->first, 1);
      }
    }
  }
}

void SweetPieScript::EquipItem(MpActor& actor, uint32_t baseId,
                               bool preventRemoval, bool silent)
{
  bool isShield = baseId == 0x7F47DC9;
  bool isArrow = baseId == 0x010B0A7;
  std::string type = "weapon";
  if (isShield) {
    type = "armor";
  }
  if (isArrow) {
    type = "ammo";
  }
  std::stringstream ss;
  ss << "[" << nlohmann::json{ { "formId", baseId }, { "type", type } }.dump()
     << ", " << (preventRemoval ? "true" : "false") << ", "
     << (silent ? "true" : "false") << "]";
  std::string args = ss.str();
  spdlog::info("Equipping item: {}", args);
  SpSnippet("Actor", "EquipItem", args.data(), actor.GetFormId())
    .Execute(&actor, SpSnippetMode::kNoReturnResult);
  if (!isShield && !isArrow) {
    SpSnippet("Actor", "DrawWeapon", "[]", actor.GetFormId())
      .Execute(&actor, SpSnippetMode::kNoReturnResult);
  }
}
