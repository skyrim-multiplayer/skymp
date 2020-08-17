#pragma once
#include "FormIndex.h"
#include "Grid.h"
#include "Inventory.h"
#include "JsonUtils.h"
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

class MpObjectReference
  : public MpForm
  , protected LocationalData
  , public FormIndex
{
public:
  using SubscribeCallback =
    std::function<void(MpObjectReference* emitter, MpActor* listener)>;

  MpObjectReference(const LocationalData& locationalData_,
                    const SubscribeCallback& onSubscribe_,
                    const SubscribeCallback& onUnsubscribe_, uint32_t baseId);

  const auto& GetPos() const { return pos; }
  const auto& GetAngle() const { return rot; }
  const auto& GetCellOrWorld() const { return cellOrWorld; }
  const auto& GetBaseId() const { return baseId; }
  const auto& GetInventory() const { return inv; }
  const auto& IsHarvested() const { return isHarvested; }
  const auto& GetRelootTime() const { return relootTime; }

  using PropertiesVisitor =
    std::function<void(const char* propName, const char* jsonValue)>;

  void VisitProperties(const PropertiesVisitor& visitor);

  void SetPos(const NiPoint3& newPos);
  void SetAngle(const NiPoint3& newAngle);
  void SetHarvested(bool harvested);
  void Activate(MpActor& activationSource, espm::Loader& loader,
                espm::CompressedFieldsCache& compressedFieldsCache);
  void SetRelootTime(std::chrono::milliseconds newRelootTime);

  void AddItem(uint32_t baseId, uint32_t count);

  static void Subscribe(MpObjectReference* emitter, MpActor* listener);
  static void Unsubscribe(MpObjectReference* emitter, MpActor* listener);

  const std::set<MpActor*>& GetListeners() const;
  const std::set<MpObjectReference*>& GetEmitters() const;

private:
  void Init(WorldState* parent, uint32_t formId) override;

  void MoveOnGrid(GridImpl<MpObjectReference*>& grid);
  void InitListenersAndEmitters();
  void RequestReloot();

  bool everSubscribedOrListened = false;
  std::unique_ptr<std::set<MpActor*>> listeners;
  const SubscribeCallback onSubscribe, onUnsubscribe;

  // Should be empty for non-actor refs
  std::unique_ptr<std::set<MpObjectReference*>> emitters;

  Inventory inv;
  uint32_t baseId = 0;
  bool isHarvested = false;
  std::chrono::milliseconds relootTime{ 3000 };

protected:
  void BeforeDestroy() override;
};