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
#include <optional>
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

  MpObjectReference(
    const LocationalData& locationalData, const FormCallbacks& callbacks,
    uint32_t baseId, std::string baseType,
    std::optional<NiPoint3> primitiveBoundsDiv2 = std::nullopt);

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
  bool IsPointInsidePrimitive(const NiPoint3& point) const;
  bool HasPrimitive() const;
  FormCallbacks GetCallbacks() const;
  bool HasScript(const char* name) const;
  bool IsActivationBlocked() const;

  using PropertiesVisitor =
    std::function<void(const char* propName, const char* jsonValue)>;

  virtual void VisitProperties(const PropertiesVisitor& visitor,
                               VisitPropertiesMode mode);
  virtual void Activate(MpObjectReference& activationSource,
                        bool defaultProcessingOnly = false);

  void SetPos(const NiPoint3& newPos);
  void SetAngle(const NiPoint3& newAngle);
  void SetHarvested(bool harvested);
  void SetOpen(bool open);
  void PutItem(MpActor& actor, const Inventory::Entry& entry);
  void TakeItem(MpActor& actor, const Inventory::Entry& entry);
  void SetRelootTime(std::chrono::milliseconds newRelootTime);
  void SetChanceNoneOverride(uint8_t chanceNone);
  void SetCellOrWorld(uint32_t worldOrCell);
  void SetAnimationVariableBool(const char* name, bool value);
  void Disable();
  void Enable();
  void SetActivationBlocked(bool blocked);
  void ForceSubscriptionsUpdate();
  void SetPrimitive(const NiPoint3& boundsDiv2);
  void UpdateHoster(uint32_t newHosterId);

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

  using Visitor = std::function<void(MpObjectReference*)>;
  void VisitNeighbours(const Visitor& visitor);

protected:
  void SendPapyrusEvent(const char* eventName,
                        const VarValue* arguments = nullptr,
                        size_t argumentsCount = 0) override;

private:
  void Init(WorldState* parent, uint32_t formId, bool hasChangeForm) override;

  void InitScripts();
  void MoveOnGrid(GridImpl<MpObjectReference*>& grid);
  void InitListenersAndEmitters();
  void SendInventoryUpdate();
  void SendOpenContainer(uint32_t refId);
  void EnsureBaseContainerAdded(espm::Loader& espm);
  void CheckInteractionAbility(MpObjectReference& ac);
  void SendPropertyToListeners(const char* name, const nlohmann::json& value);
  void SendPropertyTo(const char* name, const nlohmann::json& value,
                      MpActor& target);
  void SendPropertyTo(const std::string& preparedPropMsg, MpActor& target);
  bool IsLocationSavingNeeded() const;
  void ProcessActivate(MpObjectReference& activationSource);

  bool everSubscribedOrListened = false;
  std::unique_ptr<std::set<MpObjectReference*>> listeners;

  // Should be empty for non-actor refs
  std::unique_ptr<std::set<MpObjectReference*>> emitters;
  std::unique_ptr<std::map<uint32_t, bool>> emittersWithPrimitives;
  std::unique_ptr<std::set<uint32_t>> primitivesWeAreInside;

  std::string baseType;
  uint32_t baseId = 0;
  MpActor* occupant = nullptr;
  std::shared_ptr<OccupantDestroyEventSink> occupantDestroySink;
  std::chrono::milliseconds relootTime{ 3000 };
  std::unique_ptr<uint8_t> chanceNoneOverride;
  bool activationBlocked = false;

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