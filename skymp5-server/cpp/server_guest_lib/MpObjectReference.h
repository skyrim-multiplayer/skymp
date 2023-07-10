#pragma once
#include "ChangeFormGuard.h"
#include "FormIndex.h"
#include "Grid.h"
#include "IWorldObject.h"
#include "Inventory.h"
#include "JsonUtils.h"
#include "LocationalData.h"
#include "MpChangeForms.h"
#include "MpForm.h"
#include "libespm/Loader.h"
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
  , public IWorldObject
  , protected ChangeFormGuard
{
  friend class OccupantDestroyEventSink;

public:
  static const char* Type() { return "ObjectReference"; }
  const char* GetFormType() const override { return "ObjectReference"; }

  MpObjectReference(
    const LocationalData& locationalData, const FormCallbacks& callbacks,
    uint32_t baseId, std::string baseType,
    std::optional<NiPoint3> primitiveBoundsDiv2 = std::nullopt);

  const NiPoint3& GetPos() const override;
  const NiPoint3& GetAngle() const override;
  const FormDesc& GetCellOrWorld() const override;
  const uint32_t& GetBaseId() const;
  const std::string& GetBaseType() const;
  const Inventory& GetInventory() const;
  const bool& IsHarvested() const;
  const bool& IsOpen() const;
  const bool& IsDisabled() const;
  std::chrono::system_clock::duration GetRelootTime() const;
  bool GetAnimationVariableBool(const char* name) const;
  bool IsPointInsidePrimitive(const NiPoint3& point) const;
  bool HasPrimitive() const;
  FormCallbacks GetCallbacks() const;
  bool HasScript(const char* name) const;
  bool IsActivationBlocked() const;
  bool GetTeleportFlag() const;

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
  void SetRelootTime(std::chrono::system_clock::duration newRelootTime);
  void SetChanceNoneOverride(uint8_t chanceNone);
  void SetCellOrWorld(const FormDesc& worldOrCell);
  void SetAnimationVariableBool(const char* name, bool value);
  void Disable();
  void Enable();
  void SetActivationBlocked(bool blocked);
  void ForceSubscriptionsUpdate();
  void SetPrimitive(const NiPoint3& boundsDiv2);
  void UpdateHoster(uint32_t newHosterId);
  void SetProperty(const std::string& propertyName,
                   const nlohmann::json& newValue, bool isVisibleByOwner,
                   bool isVisibleByNeighbor);
  void SetTeleportFlag(bool value);
  void SetPosAndAngleSilent(const NiPoint3& pos, const NiPoint3& rot);

  // If you want to completely remove ObjectReference from the grid you need
  // toUnsubscribeFromAll and then RemoveFromGrid. Do not use any of these
  // functions without another in new code if you have no good reason for this.
  void UnsubscribeFromAll();
  void RemoveFromGrid();

  void SetInventory(const Inventory& inv);
  void AddItem(uint32_t baseId, uint32_t count);
  void AddItems(const std::vector<Inventory::Entry>& entries);
  void RemoveItem(uint32_t baseId, uint32_t count, MpObjectReference* target);
  void RemoveItems(const std::vector<Inventory::Entry>& entries,
                   MpObjectReference* target = nullptr);
  void RemoveAllItems(MpObjectReference* target = nullptr);
  void RelootContainer();
  void RegisterProfileId(int32_t profileId);

  static void Subscribe(MpObjectReference* emitter,
                        MpObjectReference* listener);
  static void Unsubscribe(MpObjectReference* emitter,
                          MpObjectReference* listener);

  const std::set<MpObjectReference*>& GetListeners() const;
  const std::set<MpObjectReference*>& GetEmitters() const;

  // uses default reloot time if nullopt passed
  void RequestReloot(
    std::optional<std::chrono::system_clock::duration> time = std::nullopt);
  void DoReloot();
  std::shared_ptr<std::chrono::time_point<std::chrono::system_clock>>
  GetNextRelootMoment() const;

  virtual MpChangeForm GetChangeForm() const;
  virtual void ApplyChangeForm(const MpChangeForm& changeForm);
  const DynamicFields& GetDynamicFields() const;

  // This method removes ObjectReference from a current grid and doesn't attach
  // to another grid
  void SetCellOrWorldObsolete(const FormDesc& worldOrCell);

  using Visitor = std::function<void(MpObjectReference*)>;
  void VisitNeighbours(const Visitor& visitor);

  void SendInventoryUpdate();

protected:
  void SendPapyrusEvent(const char* eventName,
                        const VarValue* arguments = nullptr,
                        size_t argumentsCount = 0) override;
  void Init(WorldState* parent, uint32_t formId, bool hasChangeForm) override;

  void EnsureBaseContainerAdded(espm::Loader& espm);

  void SendPropertyToListeners(const char* name, const nlohmann::json& value);
  void SendPropertyTo(const char* name, const nlohmann::json& value,
                      MpActor& target);
  void SendPropertyTo(const std::string& preparedPropMsg, MpActor& target);

private:
  void AddContainerObject(const espm::CONT::ContainerObject& containerObject,
                          std::map<uint32_t, uint32_t>* itemsToAdd);
  void InitScripts();
  void MoveOnGrid(GridImpl<MpObjectReference*>& grid);
  void InitListenersAndEmitters();
  void SendOpenContainer(uint32_t refId);
  void CheckInteractionAbility(MpObjectReference& ac);
  bool IsLocationSavingNeeded() const;
  void ProcessActivate(MpObjectReference& activationSource);
  bool MpApiOnActivate(MpObjectReference& caster);

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
  std::optional<std::chrono::system_clock::duration> relootTimeOverride;
  std::unique_ptr<uint8_t> chanceNoneOverride;
  bool activationBlocked = false;

  struct Impl;
  std::shared_ptr<Impl> pImpl;

protected:
  void BeforeDestroy() override;
  std::string CreatePropertyMessage(MpObjectReference* self, const char* name,
                                    const nlohmann::json& value);
  nlohmann::json PreparePropertyMessage(MpObjectReference* self,
                                        const char* name,
                                        const nlohmann::json& value);

  const std::shared_ptr<FormCallbacks> callbacks;
};
