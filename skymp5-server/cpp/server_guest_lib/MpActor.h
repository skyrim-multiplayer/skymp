#pragma once
#include "Appearance.h"
#include "MpObjectReference.h"
#include "Structures.h"
#include <memory>
#include <optional>
#include <set>

class WorldState;

constexpr float kRespawnTimeSeconds = 5.f;
static const LocationalData kSpawnPos = { { 133857, -61130, 14662 },
                                          { 0.f, 0.f, 72.f },
                                          0x3c };

class MpActor : public MpObjectReference
{
public:
  static const char* Type() { return "Actor"; }
  const char* GetFormType() const override { return "Actor"; }

  MpActor(const LocationalData& locationalData_,
          const FormCallbacks& calbacks_, uint32_t optBaseId = 0);

  const bool& IsRaceMenuOpen() const;
  const bool& IsDead() const;
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
                       VisitPropertiesMode mode) override;

  void SendToUser(const void* data, size_t size, bool reliable);

  void OnEquip(uint32_t baseId);

  class DestroyEventSink
  {
  public:
    virtual ~DestroyEventSink() = default;
    virtual void BeforeDestroy(MpActor& actor) = 0;
  };

  void AddEventSink(std::shared_ptr<DestroyEventSink> sink);
  void RemoveEventSink(std::shared_ptr<DestroyEventSink> sink);

  MpChangeForm GetChangeForm() const override;
  void ApplyChangeForm(const MpChangeForm& changeForm) override;

  uint32_t NextSnippetIndex(
    std::optional<Viet::Promise<VarValue>> promise = std::nullopt);

  void ResolveSnippet(uint32_t snippetIdx, VarValue v);
  void SetPercentages(float healthPercentage, float magickaPercentage,
                      float staminaPercentage);

  std::chrono::steady_clock::time_point GetLastAttributesPercentagesUpdate();
  std::chrono::steady_clock::time_point GetLastHitTime();

  void SetLastAttributesPercentagesUpdate(
    std::chrono::steady_clock::time_point timePoint =
      std::chrono::steady_clock::now());
  void SetLastHitTime(std::chrono::steady_clock::time_point timePoint =
                        std::chrono::steady_clock::now());

  std::chrono::duration<float> GetDurationOfAttributesPercentagesUpdate(
    std::chrono::steady_clock::time_point now);

  void Kill();
  void RespawnAfter(float seconds, const LocationalData& position = kSpawnPos);
  void Respawn(const LocationalData& position = kSpawnPos);
  void Teleport(const LocationalData& position);

private:
  std::set<std::shared_ptr<DestroyEventSink>> destroyEventSinks;

  struct Impl;
  std::shared_ptr<Impl> pImpl;

  void SendAndSetDeathState(bool isDead);
  void SendAndSetDeathState(const LocationalData& position, bool isDead,
                            bool shouldTeleport = true);
  std::string GetDeathStateMsg(const LocationalData& position, bool isDead,
                               bool shouldTeleport);

protected:
  void BeforeDestroy() override;
  void Init(WorldState* parent, uint32_t formId, bool hasChangeForm) override;
};
