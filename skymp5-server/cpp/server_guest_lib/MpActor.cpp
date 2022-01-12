#include "MpActor.h"
#include "ChangeFormGuard.h"
#include "EspmGameObject.h"
#include "FormCallbacks.h"
#include "GetBaseActorValues.h"
#include "MsgType.h"
#include "ServerState.h"
#include "WorldState.h"
#include <NiPoint3.h>
#include <random>
#include <string>

std::random_device rd;
std::mt19937 gen(rd());

struct MpActor::Impl : public ChangeFormGuard<MpChangeForm>
{
  Impl(MpChangeForm changeForm_, MpObjectReference* self_)
    : ChangeFormGuard(changeForm_, self_)
  {
  }

  std::map<uint32_t, Viet::Promise<VarValue>> snippetPromises;
  uint32_t snippetIndex = 0;
  bool isRespawning = false;
  std::chrono::steady_clock::time_point lastAttributesUpdateTimePoint,
    lastHitTimePoint;
};

MpActor::MpActor(const LocationalData& locationalData_,
                 const FormCallbacks& callbacks_, uint32_t optBaseId)
  : MpObjectReference(locationalData_, callbacks_,
                      optBaseId == 0 ? 0x7 : optBaseId, "NPC_")
{
  pImpl.reset(new Impl{ MpChangeForm(), this });
}

void MpActor::SetRaceMenuOpen(bool isOpen)
{
  pImpl->EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.isRaceMenuOpen = isOpen; });
}

void MpActor::SetAppearance(const Appearance* newAppearance)
{
  pImpl->EditChangeForm([&](MpChangeForm& changeForm) {
    if (newAppearance)
      changeForm.appearanceDump = newAppearance->ToJson();
    else
      changeForm.appearanceDump.clear();
  });
}

void MpActor::SetEquipment(const std::string& jsonString)
{
  pImpl->EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.equipmentDump = jsonString; });
}

void MpActor::VisitProperties(const PropertiesVisitor& visitor,
                              VisitPropertiesMode mode)
{
  auto baseId = MpObjectReference::GetBaseId();
  uint32_t raceId = GetAppearance() ? GetAppearance()->raceId : 0;
  BaseActorValues baseActorValues;
  WorldState* worldState = GetParent();
  // this "if" is needed for unit testing: tests can call VisitProperties
  // without espm attached, which will cause tests to fail
  if (worldState && worldState->HasEspm()) {
    baseActorValues = GetBaseActorValues(worldState, baseId, raceId);
  }

  MpChangeForm changeForm = GetChangeForm();

  MpObjectReference::VisitProperties(visitor, mode);
  if (mode == VisitPropertiesMode::All && IsRaceMenuOpen())
    visitor("isRaceMenuOpen", "true");

  if (mode == VisitPropertiesMode::All) {
    baseActorValues.VisitBaseActorValues(baseActorValues, changeForm, visitor);
  }
}

void MpActor::SendToUser(const void* data, size_t size, bool reliable)
{
  if (callbacks->sendToUser)
    callbacks->sendToUser(this, data, size, reliable);
  else
    throw std::runtime_error("sendToUser is nullptr");
}

std::unordered_map<int, std::vector<uint32_t>> weapTable = {
  {
    1,
    { 0x0001397E, 0x00013790, 0x00013981, 0x0002C66F, 0x0001CB64, 0x00012EB7,
      0x00013980, 0x000CADE9, 0x0002C672, 0x0002E6D1, 0x0001359D, 0x00013982,
      0x000CC829, 0x000236A5, 0x000302CD },
  },
};

std::unordered_map<std::string, std::unordered_map<int, std::vector<uint32_t>>>
  lootTable = {
    { "WEAP",
      {
        {
          1,
          { 0x0001397E, 0x00013790, 0x00013981, 0x0002C66F, 0x0001CB64,
            0x00012EB7, 0x00013980, 0x000CADE9, 0x0002C672, 0x0002E6D1,
            0x0001359D, 0x00013982, 0x000CC829, 0x000236A5, 0x000302CD },
        },
        { 2, { 0x00013986, 0x00013983, 0x0001398A, 0x00013997, 0x00013998,
               0x000139A1, 0x0001399C, 0x0001398E, 0x0001398B, 0x00013992,
               0x00013989, 0x00013984, 0x00013996, 0x00013993, 0x0001399A,
               0x0001399F, 0x000139A0, 0x00013991, 0x0001398C, 0x0010AA19,
               0x00013987, 0x00013988, 0x00013999, 0x00013994, 0x0001399E,
               0x0001399B, 0x000139A2, 0x0001398F, 0x00013990, 0x0010C6FB } },
      } },
    { "GEAR",
      {
        { 1, { 0x00013913, 0x00013922, 0x00012E4D, 0x0001B3A1, 0x0006F39E,
               0x000D8D52, 0x00056A9E, 0x00012E46, 0x00013912, 0x000D8D55,
               0x00012EB6, 0x000209A6, 0x0006ff38, 0x0005B69F, 0x0005b6a1,
               0x0006FF45, 0x000209A5, 0x000209A5, 0x000C5D12, 0x00013911,
               0x0003619E, 0x00012E49, 0x0001B3A3, 0x0001B3A4, 0x0010594D,
               0x000D8D50, 0x00018388, 0x00013921, 0x00013921, 0x0001394B,
               0x0001be1a, 0x000f1229, 0x000209a5, 0x0006c1d9, 0x0004223C,
               0x0001BE1B, 0x0001BE1B, 0x000BACD7, 0x0006FF37, 0x00013910,
               0x00013920, 0x00012E4B, 0x0001B39F, 0x0006F398, 0x000D8D4E,
               0x00056a9D, 0x0006F39B, 0x0001B3A0, 0x00013914, 0x0005C06C,
               0x0006c1d9, 0x0006ff43, 0x0006C1D8, 0x0003452E, 0x000D191F,
               0x0003452F, 0x000C36E9 } },
      } },
    { "CONS",
      {
        { 1, { 0x0004B0BA, 0x00034CDF, 0x0005076E, 0x0001D4EC } },
        { 2, { 0x0006AC4A, 0x0001B3BD, 0x00064B2E, 0x00064B2F, 0x00023D77 } },
        { 3, { 0x0000353C } }, // change to dlc id 0xXX00353c
        { 4, { 0x0003EADE } },
        { 5, { 0x002E504 } },
      } }
  };

int GenerateRandomNumber(int leftBound, int rightBound)
{
  std::uniform_int_distribution<> distr(leftBound, rightBound);
  return distr(gen);
}

std::string AcknowledgeType(int weaponChance, int gearChance,
                            int consumablesChance, int nothingChance)
{

  int chance = GenerateRandomNumber(1, 100);
  std::string type = "";

  if (chance <= weaponChance && weaponChance != 0) {
    type = "WEAP";
    return type;
  } else if (chance <= (weaponChance + gearChance) && gearChance != 0) {
    type = "GEAR";
    return type;
  } else if (chance <= (weaponChance + gearChance + consumablesChance) &&
             consumablesChance != 0) {
    type = "CONS";
    return type;
  } else {
    type = "NOTH";
    return type;
  }
}

int AcknowledgeTier(int tier1Chance, int tier2Chance, int tier3Chance,
                    int tier4Chance, int tier5Chance)
{
  int chance = GenerateRandomNumber(1, 100);

  if (chance < tier1Chance && tier1Chance != 0) {
    return 1;
  } else if (chance <= (tier1Chance + tier2Chance) && tier2Chance != 0) {
    return 2;
  } else if (chance <= (tier1Chance + tier2Chance + tier3Chance) &&
             tier3Chance != 0) {
    return 3;
  } else if (chance <=
               (tier1Chance + tier2Chance + tier3Chance + tier4Chance) &&
             tier4Chance != 0) {
    return 4;
  } else {
    return 5;
  }
}

int GenerateItemIndex(int tier, std::string type)
{
  if (type == "WEAP") {
    switch (tier) {
      case 1:
        return GenerateRandomNumber(0, 14);
        break;
      case 2:
        return GenerateRandomNumber(0, 29);
        break;
      case 3:
        return GenerateRandomNumber(0, 22);
        break;
      case 4:
        return GenerateRandomNumber(0, 28);
        break;
      case 5:
        return GenerateRandomNumber(0, 17);
        break;
    }
  } else if (type == "GEAR") {
    switch (tier) {
      case 1:
        return GenerateRandomNumber(0, 49);
        break;
      case 2:
        return GenerateRandomNumber(0, 58);
        break;
      case 3:
        return GenerateRandomNumber(0, 65);
        break;
      case 4:
        return GenerateRandomNumber(0, 36);
        break;
      case 5:
        return GenerateRandomNumber(0, 14);
        break;
    }
  } else if (type == "CONS") {
    switch (tier) {
      case 1:
        return GenerateRandomNumber(0, 3);
        break;
      case 2:
        return GenerateRandomNumber(0, 4);
        break;
      case 3:
        return GenerateRandomNumber(0, 0);
        break;
      case 4:
        return GenerateRandomNumber(0, 0);
        break;
      case 5:
        return GenerateRandomNumber(0, 0);
        break;
    }
  }
}

uint32_t GetRandomItem(std::string type)
{
  if (type == "WEAP") {
    int tier = AcknowledgeTier(60, 20, 10, 9, 1);
    int item = GenerateItemIndex(tier, type);
    return lootTable[type][tier][item];
  } else if (type == "GEAR") {
    int tier = AcknowledgeTier(60, 20, 10, 9, 1);
    int item = GenerateItemIndex(tier, type);
    return lootTable[type][tier][item];
  } else if (type == "CONS") {
    int tier = AcknowledgeTier(60, 20, 10, 9, 1);
    int item = GenerateItemIndex(tier, type);
    return lootTable[type][tier][item];
  } else if (type == "NOTH") {
      // do smth
  } 
}

uint32_t GetSlotItem(int weaponChance, int gearChance, int consumableChance,
                     int nothingChance)
{
  std::string type =
    AcknowledgeType(weaponChance, gearChance, consumableChance, nothingChance);
  GetRandomItem(type);
}

void MpActor::OnEquip(uint32_t baseId)
{
  if (GetInventory().GetItemCount(baseId) == 0)
    return;
  auto& espm = GetParent()->GetEspm();
  auto lookupRes = espm.GetBrowser().LookupById(baseId);
  if (!lookupRes.rec)
    return;
  auto t = lookupRes.rec->GetType();
  if (t == "INGR" || t == "ALCH") {
    // Eat item
    RemoveItem(baseId, 1, nullptr);

    VarValue args[] = { VarValue(std::make_shared<EspmGameObject>(lookupRes)),
                        VarValue::None() };
    SendPapyrusEvent("OnObjectEquipped", args, std::size(args));

    if (baseId == 0x00064B43) {
       AddItem(GetSlotItem(80, 10, 8, 2), 1);
       AddItem(GetSlotItem(10, 80, 8, 2), 1);
       AddItem(GetSlotItem(10, 50, 30, 10), 1);
       AddItem(GetSlotItem(0, 0, 94, 6), 1);
    }
  }
}

void MpActor::AddEventSink(std::shared_ptr<DestroyEventSink> sink)
{
  destroyEventSinks.insert(sink);
}

void MpActor::RemoveEventSink(std::shared_ptr<DestroyEventSink> sink)
{
  destroyEventSinks.erase(sink);
}

MpChangeForm MpActor::GetChangeForm() const
{
  auto res = MpObjectReference::GetChangeForm();
  auto& achr = pImpl->ChangeForm();
  res.appearanceDump = achr.appearanceDump;
  res.isRaceMenuOpen = achr.isRaceMenuOpen;
  res.equipmentDump = achr.equipmentDump;
  res.healthPercentage = achr.healthPercentage;
  res.magickaPercentage = achr.magickaPercentage;
  res.staminaPercentage = achr.staminaPercentage;
  res.isDead = achr.isDead;
  res.spawnPoint = achr.spawnPoint;
  res.spawnDelay = achr.spawnDelay;
  // achr.dynamicFields isn't really used so I decided to comment this line:
  // res.dynamicFields.merge_patch(achr.dynamicFields);

  res.recType = MpChangeForm::ACHR;
  return res;
}

void MpActor::ApplyChangeForm(const MpChangeForm& newChangeForm)
{
  if (newChangeForm.recType != MpChangeForm::ACHR) {
    throw std::runtime_error(
      "Expected record type to be ACHR, but found REFR");
  }
  MpObjectReference::ApplyChangeForm(newChangeForm);
  pImpl->EditChangeForm(
    [&](MpChangeForm& cf) {
      cf = static_cast<const MpChangeForm&>(newChangeForm);

      // Actor without appearance would not be visible so we force player to
      // choose appearance
      if (cf.appearanceDump.empty())
        cf.isRaceMenuOpen = true;
    },
    Impl::Mode::NoRequestSave);
}

uint32_t MpActor::NextSnippetIndex(
  std::optional<Viet::Promise<VarValue>> promise)
{
  auto res = pImpl->snippetIndex++;
  if (promise)
    pImpl->snippetPromises[res] = *promise;
  return res;
}

void MpActor::ResolveSnippet(uint32_t snippetIdx, VarValue v)
{
  auto it = pImpl->snippetPromises.find(snippetIdx);
  if (it != pImpl->snippetPromises.end()) {
    auto& promise = it->second;
    promise.Resolve(v);
    pImpl->snippetPromises.erase(it);
  }
}

void MpActor::SetPercentages(float healthPercentage, float magickaPercentage,
                             float staminaPercentage, MpActor* aggressor)
{
  if (IsDead() || pImpl->isRespawning) {
    return;
  }
  if (healthPercentage == 0.f) {
    Kill(aggressor);
    return;
  }
  pImpl->EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.healthPercentage = healthPercentage;
    changeForm.magickaPercentage = magickaPercentage;
    changeForm.staminaPercentage = staminaPercentage;
  });
}

std::chrono::steady_clock::time_point
MpActor::GetLastAttributesPercentagesUpdate()
{
  return pImpl->lastAttributesUpdateTimePoint;
}

std::chrono::steady_clock::time_point MpActor::GetLastHitTime()
{
  return pImpl->lastHitTimePoint;
}

void MpActor::SetLastAttributesPercentagesUpdate(
  std::chrono::steady_clock::time_point timePoint)
{
  pImpl->lastAttributesUpdateTimePoint = timePoint;
}

void MpActor::SetLastHitTime(std::chrono::steady_clock::time_point timePoint)
{
  pImpl->lastHitTimePoint = timePoint;
}

std::chrono::duration<float> MpActor::GetDurationOfAttributesPercentagesUpdate(
  std::chrono::steady_clock::time_point now)
{
  std::chrono::duration<float> timeAfterRegeneration =
    now - pImpl->lastAttributesUpdateTimePoint;
  return timeAfterRegeneration;
}

const bool& MpActor::IsRaceMenuOpen() const
{
  return pImpl->ChangeForm().isRaceMenuOpen;
}

const bool& MpActor::IsDead() const
{
  return pImpl->ChangeForm().isDead;
}

const bool& MpActor::IsRespawning() const
{
  return pImpl->isRespawning;
}

std::unique_ptr<const Appearance> MpActor::GetAppearance() const
{
  auto& changeForm = pImpl->ChangeForm();
  if (changeForm.appearanceDump.size() > 0) {
    simdjson::dom::parser p;
    auto doc = p.parse(changeForm.appearanceDump).value();

    std::unique_ptr<const Appearance> res;
    res.reset(new Appearance(Appearance::FromJson(doc)));
    return res;
  }
  return nullptr;
}

const std::string& MpActor::GetAppearanceAsJson()
{
  return pImpl->ChangeForm().appearanceDump;
}

const std::string& MpActor::GetEquipmentAsJson() const
{
  return pImpl->ChangeForm().equipmentDump;
}

Equipment MpActor::GetEquipment() const
{
  std::string equipment = GetEquipmentAsJson();
  simdjson::dom::parser p;
  auto parseResult = p.parse(equipment);
  return Equipment::FromJson(parseResult.value());
}

uint32_t MpActor::GetRaceId() const
{
  auto appearance = GetAppearance();
  if (appearance) {
    return appearance->raceId;
  }
  WorldState* espmProvider = GetParent();
  uint32_t baseId = GetBaseId();
  return espm::GetData<espm::NPC_>(baseId, espmProvider).race;
}

bool MpActor::IsWeaponDrawn() const
{
  return GetAnimationVariableBool("_skymp_isWeapDrawn");
}

espm::ObjectBounds MpActor::GetBounds() const
{
  return espm::GetData<espm::NPC_>(GetBaseId(), GetParent()).objectBounds;
}

void MpActor::SendAndSetDeathState(bool isDead, bool shouldTeleport)
{
  float attribute = isDead ? 0.f : 1.f;
  auto position = GetSpawnPoint();

  std::string respawnMsg = GetDeathStateMsg(position, isDead, shouldTeleport);
  SendToUser(respawnMsg.data(), respawnMsg.size(), true);

  pImpl->EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.isDead = isDead;
    changeForm.healthPercentage = attribute;
    changeForm.magickaPercentage = attribute;
    changeForm.staminaPercentage = attribute;
  });
  if (shouldTeleport) {
    SetCellOrWorldObsolete(position.cellOrWorldDesc);
    SetPos(position.pos);
    SetAngle(position.rot);
  }
}

std::string MpActor::GetDeathStateMsg(const LocationalData& position,
                                      bool isDead, bool shouldTeleport)
{
  nlohmann::json tTeleport = nlohmann::json{};
  nlohmann::json tChangeValues = nlohmann::json{};
  nlohmann::json tIsDead = PreparePropertyMessage(this, "isDead", isDead);

  if (shouldTeleport) {
    tTeleport = nlohmann::json{
      { "pos", { position.pos[0], position.pos[1], position.pos[2] } },
      { "rot", { position.rot[0], position.rot[1], position.rot[2] } },
      { "worldOrCell",
        position.cellOrWorldDesc.ToFormId(GetParent()->espmFiles) },
      { "type", "teleport" }
    };
  }
  if (isDead == false) {
    const float attribute = 1.f;
    tChangeValues = nlohmann::json{ { "t", MsgType::ChangeValues },
                                    { "data",
                                      { { "health", attribute },
                                        { "magicka", attribute },
                                        { "stamina", attribute } } } };
  }

  std::string DeathStateMsg;
  DeathStateMsg += Networking::MinPacketId;
  DeathStateMsg += nlohmann::json{
    { "t", MsgType::DeathStateContainer },
    { "tTeleport", tTeleport },
    { "tChangeValues", tChangeValues },
    { "tIsDead", tIsDead }
  }.dump();
  return DeathStateMsg;
}

void MpActor::MpApiDeath(MpActor* killer)
{
  simdjson::dom::parser parser;
  bool isRespawnBlocked = false;

  std::string s =
    "[" + std::to_string(killer ? killer->GetFormId() : 0) + " ]";
  auto args = parser.parse(s).value();

  if (auto wst = GetParent()) {
    const auto id = GetFormId();
    for (auto& listener : wst->listeners) {
      if (listener->OnMpApiEvent("onDeath", args, id) == false) {
        isRespawnBlocked = true;
      };
    }
  }
  if (!isRespawnBlocked) {
    RespawnWithDelay();
  }
}

void MpActor::BeforeDestroy()
{
  for (auto& sink : destroyEventSinks)
    sink->BeforeDestroy(*this);

  MpObjectReference::BeforeDestroy();

  UnsubscribeFromAll();
}

void MpActor::Init(WorldState* worldState, uint32_t formId, bool hasChangeForm)
{
  MpObjectReference::Init(worldState, formId, hasChangeForm);

  if (worldState->HasEspm()) {
    EnsureBaseContainerAdded(GetParent()->GetEspm());
  }
}

void MpActor::Kill(MpActor* killer, bool shouldTeleport)
{
  SendAndSetDeathState(true, shouldTeleport);
  MpApiDeath(killer);
}

void MpActor::RespawnWithDelay(bool shouldTeleport)
{
  if (pImpl->isRespawning) {
    return;
  }
  pImpl->isRespawning = true;

  uint32_t formId = GetFormId();
  if (auto worldState = GetParent()) {
    worldState->SetTimer(GetRespawnTime())
      .Then([worldState, this, formId, shouldTeleport](Viet::Void) {
        if (worldState->LookupFormById(formId).get() == this) {
          this->Respawn(shouldTeleport);
        }
      });
  }
}

void MpActor::Respawn(bool shouldTeleport)
{
  if (IsDead() == false) {
    return;
  }
  pImpl->isRespawning = false;
  SendAndSetDeathState(false, shouldTeleport);
}

void MpActor::Teleport(const LocationalData& position)
{
  std::string teleportMsg;
  teleportMsg += Networking::MinPacketId;
  teleportMsg += nlohmann::json{
    { "pos", { position.pos[0], position.pos[1], position.pos[2] } },
    { "rot", { position.rot[0], position.rot[1], position.rot[2] } },
    { "worldOrCell",
      position.cellOrWorldDesc.ToFormId(GetParent()->espmFiles) },
    { "type", "teleport" }
  }.dump();
  SendToUser(teleportMsg.data(), teleportMsg.size(), true);

  SetCellOrWorldObsolete(position.cellOrWorldDesc);
  SetPos(position.pos);
  SetAngle(position.rot);
}

void MpActor::SetSpawnPoint(const LocationalData& position)
{
  pImpl->EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.spawnPoint = position; });
}

LocationalData MpActor::GetSpawnPoint() const
{
  return pImpl->ChangeForm().spawnPoint;
}

const float MpActor::GetRespawnTime() const
{
  return pImpl->ChangeForm().spawnDelay;
}

void MpActor::SetRespawnTime(float time)
{
  pImpl->EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.spawnDelay = time; });
}

void MpActor::SetIsDead(bool isDead)
{
  SendAndSetDeathState(isDead, false);
}
