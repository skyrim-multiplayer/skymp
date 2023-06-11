#pragma once
#include "Appearance.h"
#include "GetBaseActorValues.h"
#include "MpObjectReference.h"
#include "Structures.h"
#include <memory>
#include <optional>
#include <set>

class WorldState;
struct ActorValues;

class MpActor final
{
  using Visitor = std::function<void(MpObjectReference*)>;
  using PropertiesVisitor =
    std::function<void(const char* propName, const char* jsonValue)>;

public:
  static const char* Type() { return "Actor"; }
  const char* GetFormType() const { return "Actor"; }

  MpActor();

  const bool& IsRespawning() const;
  std::unique_ptr<const Appearance> GetAppearance() const;
  const std::string& GetAppearanceAsJson();
  const std::string& GetEquipmentAsJson() const;
  Equipment GetEquipment() const;
  uint32_t GetRaceId() const;
  bool IsWeaponDrawn() const;
  espm::ObjectBounds GetBounds() const;
  void SetRaceMenuOpen(bool isOpen);
  void SetAppearance(const Appearance* newAppearance);
  void SetEquipment(const std::string& jsonString);
  void VisitProperties(const PropertiesVisitor& visitor,
                       VisitPropertiesMode mode);

  void SendToUser(const void* data, size_t size, bool reliable);

  class DestroyEventSink
  {
  public:
    virtual ~DestroyEventSink() = default;
    virtual void BeforeDestroy(MpActor& actor) = 0;
  };

  void AddEventSink(std::shared_ptr<DestroyEventSink> sink);
  void RemoveEventSink(std::shared_ptr<DestroyEventSink> sink);
  MpChangeForm GetChangeForm() const;
  void ApplyChangeForm(const MpChangeForm& changeForm);
  uint32_t NextSnippetIndex(
    std::optional<Viet::Promise<VarValue>> promise = std::nullopt);
  void ResolveSnippet(uint32_t snippetIdx, VarValue v);
  void SetPercentages(const ActorValues& actorValues,
                      MpActor* aggressor = nullptr);
  void NetSendChangeValues(const ActorValues& actorValues);
  void NetSetPercentages(const ActorValues& actorValues,
                         MpActor* aggressor = nullptr);
  std::chrono::steady_clock::time_point GetLastAttributesPercentagesUpdate();
  std::chrono::steady_clock::time_point GetLastHitTime();
  void SetLastAttributesPercentagesUpdate(
    std::chrono::steady_clock::time_point timePoint =
      std::chrono::steady_clock::now());
  void SetLastHitTime(std::chrono::steady_clock::time_point timePoint =
                        std::chrono::steady_clock::now());
  std::chrono::duration<float> GetDurationOfAttributesPercentagesUpdate(
    std::chrono::steady_clock::time_point now);
  void Kill(MpActor* killer = nullptr, bool shouldTeleport = false);
  void Respawn(bool shouldTeleport = true);
  void RespawnWithDelay(bool shouldTeleport = true);
  void Teleport(const LocationalData& position);
  void SetSpawnPoint(const LocationalData& position);
  LocationalData GetSpawnPoint() const;
  const float GetRespawnTime() const;
  void SetRespawnTime(float time);
  void SetIsDead(bool isDead);
  void RestoreActorValue(espm::ActorValue av, float value);
  void DamageActorValue(espm::ActorValue av, float value);
  void SetActorValue(espm::ActorValue actorValue, float value);
  BaseActorValues GetBaseValues();
  BaseActorValues GetMaximumValues();
  void DropItem(const uint32_t baseId, const Inventory::Entry& entry);
  void SetIsBlockActive(bool isBlockActive);
  bool IsBlockActive() const;
  NiPoint3 GetViewDirection() const;
  bool CanActorValueBeRestored(espm::ActorValue av);

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
  void SendAndSetDeathState(bool isDead, bool shouldTeleport);
  std::string GetDeathStateMsg(const LocationalData& position, bool isDead,
                               bool shouldTeleport);
  void MpApiDeath(MpActor* killer = nullptr);
  void EatItem(uint32_t baseId, espm::Type t);
  void ModifyActorValuePercentage(espm::ActorValue av, float percentageDelta);
  std::chrono::steady_clock::time_point GetLastRestorationTime(
    espm::ActorValue av) const;
  void SetLastRestorationTime(espm::ActorValue av,
                              std::chrono::steady_clock::time_point timePoint);

protected:
  void BeforeDestroy();
  void Init(WorldState* parent, uint32_t formId, bool hasChangeForm);
};
