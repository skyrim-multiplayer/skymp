#pragma once
#include "FormIndex.h"
#include "JsonUtils.h"
#include "MpForm.h"
#include <nlohmann/json.hpp>
#include <set>
#include <simdjson.h>
#include <vector>

class WorldState;

struct LocationalData
{
  NiPoint3 pos, rot;
  uint32_t cellOrWorld;
};

class MpActor
  : public MpForm
  , private LocationalData
  , public FormIndex
{
public:
  using SubscribeCallback =
    std::function<void(MpActor* emitter, MpActor* listener)>;

  struct Tint
  {
    static Tint FromJson(simdjson::dom::element& j);

    std::string texturePath;
    int32_t argb = 0;
    int32_t type = 0;
  };

  struct Look
  {
    static Look FromJson(const nlohmann::json& j);
    static Look FromJson(simdjson::dom::element& j);
    std::string ToJson() const;

    bool isFemale = false;
    uint32_t raceId = 0;
    float weight = 0.f;
    int32_t skinColor = 0;
    int32_t hairColor = 0;
    std::vector<uint32_t> headpartIds;
    uint32_t headTextureSetId = 0;
    std::vector<float> faceMorphs;
    std::vector<float> facePresets;
    std::vector<Tint> tints;
    std::string name;
  };

  MpActor(const LocationalData& locationalData_,
          const SubscribeCallback& onSubscribe_,
          const SubscribeCallback& onUnsubscribe_)
    : onSubscribe(onSubscribe_)
    , onUnsubscribe(onUnsubscribe_)
  {
    static_cast<LocationalData&>(*this) = locationalData_;
  }

  ~MpActor() = default;

  const auto& GetPos() const { return pos; }
  const auto& GetAngle() const { return rot; }
  const auto& GetCellOrWorld() const { return cellOrWorld; }
  const auto& IsRaceMenuOpen() const { return isRaceMenuOpen; }
  auto GetLook() const { return look.get(); }

  const std::string& GetEquipmentAsJson() { return jEquipmentCache; };

  void SetPos(const NiPoint3& newPos);
  void SetAngle(const NiPoint3& newAngle);
  void SetRaceMenuOpen(bool isOpen);
  void SetLook(const Look* newLook);
  void SetEquipment(const std::string& jsonString);

  const std::string& GetLookAsJson();

  static void Subscribe(MpActor* emitter, MpActor* listener);
  static void Unsubscribe(MpActor* emitter, MpActor* listener);

  auto& GetListeners() const { return listeners; }

private:
  void BeforeDestroy() override;

  bool isOnGrid = false;
  std::set<MpActor*> listeners;
  std::set<MpActor*> emitters;
  const SubscribeCallback onSubscribe, onUnsubscribe;

  bool isRaceMenuOpen = false;
  std::unique_ptr<Look> look;

  std::string jLookCache;
  std::string jEquipmentCache;
};