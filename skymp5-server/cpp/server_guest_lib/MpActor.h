#pragma once
#include "Appearance.h"
#include "MpObjectReference.h"
#include "Structures.h"
#include <memory>
#include <optional>
#include <set>

class WorldState;

class MpActor : public MpObjectReference
{
public:
  static const char* Type() { return "Actor"; }
  const char* GetFormType() const override { return "Actor"; }

  MpActor(const LocationalData& locationalData_,
          const FormCallbacks& calbacks_, uint32_t optBaseId = 0);

  const bool& IsRaceMenuOpen() const;
  std::unique_ptr<const Appearance> GetAppearance() const;
  const std::string& GetAppearanceAsJson();
  const std::string& GetEquipmentAsJson();
  bool IsWeaponDrawn() const;

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

  void SetLastAttributesPercentagesUpdate(
    std::chrono::steady_clock::time_point timePoint =
      std::chrono::steady_clock::now());

  std::chrono::duration<float> GetDurationOfAttributesPercentagesUpdate(
    std::chrono::steady_clock::time_point now =
      std::chrono::steady_clock::now());

private:
  std::set<std::shared_ptr<DestroyEventSink>> destroyEventSinks;

  struct Impl;
  std::shared_ptr<Impl> pImpl;

  std::chrono::steady_clock::time_point lastAttributesUpdateTimePoint;

protected:
  void BeforeDestroy() override;
  void Init(WorldState* parent, uint32_t formId, bool hasChangeForm) override;
};
