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
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <simdjson.h>
#include <tuple>
#include <vector>

struct LocationalData
{
  NiPoint3 pos, rot;
  uint32_t cellOrWorld;
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

class MpObjectReference
  : public MpForm
  , public FormIndex
{
  friend class OccupantDestroyEventSink;

public:
  static const char* Type() { return "ObjectReference"; }

  using SubscribeCallback = std::function<void(MpObjectReference* emitter,
                                               MpObjectReference* listener)>;

  MpObjectReference(const LocationalData& locationalData_,
                    const SubscribeCallback& onSubscribe_,
                    const SubscribeCallback& onUnsubscribe_, uint32_t baseId,
                    const char* baseType);

  const NiPoint3& GetPos() const;
  const NiPoint3& GetAngle() const;
  const uint32_t& GetCellOrWorld() const;
  const uint32_t& GetBaseId() const;
  const Inventory& GetInventory() const;
  const bool& IsHarvested() const;
  const bool& IsOpen() const;
  const std::chrono::milliseconds& GetRelootTime() const;

  using PropertiesVisitor =
    std::function<void(const char* propName, const char* jsonValue)>;

  void VisitProperties(const PropertiesVisitor& visitor);

  void SetPos(const NiPoint3& newPos);
  void SetAngle(const NiPoint3& newAngle);
  void SetHarvested(bool harvested);
  void SetOpen(bool open);
  void Activate(MpActor& activationSource);
  void PutItem(MpActor& actor, const Inventory::Entry& entry);
  void TakeItem(MpActor& actor, const Inventory::Entry& entry);
  void SetRelootTime(std::chrono::milliseconds newRelootTime);
  void SetCellOrWorld(uint32_t worldOrCell);
  void SetChanceNoneOverride(uint8_t chanceNone);

  void AddItem(uint32_t baseId, uint32_t count);
  void AddItems(const std::vector<Inventory::Entry>& entries);
  void RemoveItems(const std::vector<Inventory::Entry>& entries,
                   MpObjectReference* target = nullptr);
  void RelootContainer();

  static void Subscribe(MpObjectReference* emitter,
                        MpObjectReference* listener);
  static void Unsubscribe(MpObjectReference* emitter,
                          MpObjectReference* listener);

  const std::set<MpObjectReference*>& GetListeners() const;
  const std::set<MpObjectReference*>& GetEmitters() const;

  void RequestReloot();
  void DoReloot();
  std::chrono::time_point<std::chrono::system_clock>* GetNextRelootMoment()
    const;

  virtual MpChangeForm GetChangeForm() const;

private:
  void Init(WorldState* parent, uint32_t formId) override;

  void MoveOnGrid(GridImpl<MpObjectReference*>& grid);
  void InitListenersAndEmitters();
  void SendInventoryUpdate();
  void SendOpenContainer(uint32_t refId);
  void EnsureBaseContainerAdded(espm::Loader& espm);
  void CheckInteractionAbility(MpActor& ac);
  void SendPropertyToListeners(const char* name, const nlohmann::json& value);
  void SendPropertyTo(const char* name, const nlohmann::json& value,
                      MpActor& target);

  bool everSubscribedOrListened = false;
  std::unique_ptr<std::set<MpObjectReference*>> listeners;
  const SubscribeCallback onSubscribe, onUnsubscribe;

  // Should be empty for non-actor refs
  std::unique_ptr<std::set<MpObjectReference*>> emitters;

  uint32_t baseId = 0;
  const char* const baseType;
  MpActor* occupant = nullptr;
  std::shared_ptr<OccupantDestroyEventSink> occupantDestroySink;
  std::chrono::milliseconds relootTime{ 3000 };
  bool baseContainerAdded = false;
  std::unique_ptr<uint8_t> chanceNoneOverride;

  struct Impl;
  std::shared_ptr<Impl> pImpl;

protected:
  void BeforeDestroy() override;
};