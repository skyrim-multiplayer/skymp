#pragma once
#include "FormIndex.h"
#include "JsonUtils.h"
#include "MpForm.h"
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
                    const SubscribeCallback& onUnsubscribe_)
    : onSubscribe(onSubscribe_)
    , onUnsubscribe(onUnsubscribe_)
  {
    static_cast<LocationalData&>(*this) = locationalData_;
  }

  const auto& GetPos() const { return pos; }
  const auto& GetAngle() const { return rot; }
  const auto& GetCellOrWorld() const { return cellOrWorld; }

  void SetPos(const NiPoint3& newPos);
  void SetAngle(const NiPoint3& newAngle);

  static void Subscribe(MpObjectReference* emitter, MpActor* listener);
  static void Unsubscribe(MpObjectReference* emitter, MpActor* listener);

  const std::set<MpActor*>& GetListeners() const;
  const std::set<MpObjectReference*>& GetEmitters() const;

private:
  void InitListenersAndEmitters();

  bool isOnGrid = false;
  std::unique_ptr<std::set<MpActor*>> listeners;
  const SubscribeCallback onSubscribe, onUnsubscribe;

  // Should be empty for non-actor refs
  std::unique_ptr<std::set<MpObjectReference*>> emitters;

protected:
  void BeforeDestroy() override;
};