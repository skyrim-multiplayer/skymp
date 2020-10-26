#pragma once
#include "FormIndex.h"
#include "Grid.h"
#include "Inventory.h"
#include "JsonUtils.h"
#include "MpChangeForms.h"
#include "MpForm.h"
#include <Loader.h>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <simdjson.h>
#include <string>
#include <tuple>
#include <vector>

struct LocationalData
{
  NiPoint3 pos, rot;
  uint32_t cellOrWorld = 0;
};

struct GridPosInfo
{
  uint32_t worldOrCell = 0;
  int16_t x = 0;
  int16_t y = 0;

  friend bool operator==(const GridPosInfo& lhs, const GridPosInfo& rhs)
  {
    return std::make_tuple(lhs.worldOrCell, lhs.x, lhs.y) ==
      std::make_tuple(rhs.worldOrCell, rhs.x, rhs.y);
  }
};

class MpActor;
class WorldState;
class OccupantDestroyEventSink;
struct VarValue;

class FormCallbacks;

class FormCallbacks;

enum class VisitPropertiesMode
{
  OnlyPublic,
  All
};

class MpObjectReference
  : public MpForm
  , public FormIndex
{
  friend class OccupantDestroyEventSink;

public:
  static const char* Type() { return "ObjectReference"; }
  const char* GetFormType() const override { return "ObjectReference"; }

  using SubscribeCallback = std::function<void(MpObjectReference* emitter,
                                               MpObjectReference* listener)>;
  using SendToUserFn = std::function<void(MpActor* actor, const void* data,
                                          size_t size, bool reliable)>;

  MpObjectReference(const LocationalData& locationalData,
                    const FormCallbacks& callbacks, uint32_t baseId,
                    const char* baseType);

  const NiPoint3& GetPos() const;
  const NiPoint3& GetAngle() const;
  const uint32_t& GetCellOrWorld() const;
  const uint32_t& GetBaseId() const;
  const Inventory& GetInventory() const;
  const bool& IsHarvested() const;
  const bool& IsOpen() const;
  const bool& IsDisabled() const;
  const std::chrono::milliseconds& GetRelootTime() const;
  bool GetAnimationVariableBool(const char* name) const;

  using PropertiesVisitor =
    std::function<void(const char* propName, const char* jsonValue)>;

  virtual void VisitProperties(const PropertiesVisitor& visitor,
                               VisitPropertiesMode mode);

  void SetPos(const NiPoint3& newPos);
  void SetAngle(const NiPoint3& newAngle);
  void SetHarvested(bool harvested);
  void SetOpen(bool open);
  void Activate(MpActor& activationSource);
  void PutItem(MpActor& actor, const Inventory::Entry& entry);
  void TakeItem(MpActor& actor, const Inventory::Entry& entry);
  void SetRelootTime(std::chrono::milliseconds newRelootTime);
  void SetChanceNoneOverride(uint8_t chanceNone);
  void SetCellOrWorld(uint32_t worldOrCell);
  void SetAnimationVariableBool(const char* name, bool value);
  void Disable();
  void Enable();
  void ForceSubscriptionsUpdate();

  // If you want to completely remove ObjectReference from the grid you need
  // toUnsubscribeFromAll and then RemoveFromGrid. Do not use any of these
  // functions without another in new code if you have no good reason for this.
  void UnsubscribeFromAll();
  void RemoveFromGrid();

  void AddItem(uint32_t baseId, uint32_t count);
  void AddItems(const std::vector<Inventory::Entry>& entries);
  void RemoveItem(uint32_t baseId, uint32_t count, MpObjectReference* target);
  void RemoveItems(const std::vector<Inventory::Entry>& entries,
                   MpObjectReference* target = nullptr);
  void RelootContainer();
  void RegisterProfileId(int32_t profileId);

  static void Subscribe(MpObjectReference* emitter,
                        MpObjectReference* listener);
  static void Unsubscribe(MpObjectReference* emitter,
                          MpObjectReference* listener);

  const std::set<MpObjectReference*>& GetListeners() const;
  const std::set<MpObjectReference*>& GetEmitters() const;

  void RequestReloot();
  void DoReloot();
  std::shared_ptr<std::chrono::time_point<std::chrono::system_clock>>
  GetNextRelootMoment() const;

  virtual MpChangeForm GetChangeForm() const;
  virtual void ApplyChangeForm(const MpChangeForm& changeForm);

  // This method removes ObjectReference from a current grid and doesn't attach
  // to another grid
  void SetCellOrWorldObsolete(uint32_t worldOrCell);

private:
  void Init(WorldState* parent, uint32_t formId) override;

  void InitScripts();
  void MoveOnGrid(GridImpl<MpObjectReference*>& grid);
  void InitListenersAndEmitters();
  void SendInventoryUpdate();
  void SendOpenContainer(uint32_t refId);
  void EnsureBaseContainerAdded(espm::Loader& espm);
  void CheckInteractionAbility(MpActor& ac);
  void SendPropertyToListeners(const char* name, const nlohmann::json& value);
  void SendPropertyTo(const char* name, const nlohmann::json& value,
                      MpActor& target);
  bool IsLocationSavingNeeded() const;
  bool HasScripts(); // Perform lazy loading

  bool everSubscribedOrListened = false;
  std::unique_ptr<std::set<MpObjectReference*>> listeners;

  // Should be empty for non-actor refs
  std::unique_ptr<std::set<MpObjectReference*>> emitters;

  const char* const baseType;
  uint32_t baseId = 0;
  MpActor* occupant = nullptr;
  std::shared_ptr<OccupantDestroyEventSink> occupantDestroySink;
  std::chrono::milliseconds relootTime{ 3000 };
  std::unique_ptr<uint8_t> chanceNoneOverride;

  struct Impl;
  std::shared_ptr<Impl> pImpl;

protected:
  void BeforeDestroy() override;

  const std::shared_ptr<FormCallbacks> callbacks;
};

class FormCallbacks
{
public:
  MpObjectReference::SubscribeCallback subscribe, unsubscribe;
  MpObjectReference::SendToUserFn sendToUser;

  static FormCallbacks DoNothing()
  {
    return { [](auto, auto) {}, [](auto, auto) {},
             [](auto, auto, auto, auto) {} };
  }
};