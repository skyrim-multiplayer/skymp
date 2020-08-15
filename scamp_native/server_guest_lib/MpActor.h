#pragma once
#include "MpObjectReference.h"

class WorldState;

class MpActor : public MpObjectReference
{
public:
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
    : MpObjectReference(locationalData_, onSubscribe_, onUnsubscribe_)
  {
  }

  ~MpActor() = default;

  const auto& IsRaceMenuOpen() const { return isRaceMenuOpen; }
  auto GetLook() const { return look.get(); }

  const std::string& GetEquipmentAsJson() { return jEquipmentCache; };

  void SetRaceMenuOpen(bool isOpen);
  void SetLook(const Look* newLook);
  void SetEquipment(const std::string& jsonString);

  const std::string& GetLookAsJson();

private:
  void UnsubscribeFromAll();

  bool isRaceMenuOpen = false;
  std::unique_ptr<Look> look;

  std::string jLookCache;
  std::string jEquipmentCache;

protected:
  void BeforeDestroy() override;
};