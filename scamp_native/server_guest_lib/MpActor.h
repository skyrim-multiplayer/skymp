#pragma once
#include "MpObjectReference.h"
#include <set>

class WorldState;

class MpActor : public MpObjectReference
{
public:
  static const char* Type() { return "Actor"; }

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

  constexpr static uint32_t nullBaseId = 0;

  using SendToUserFn = std::function<void(MpActor* actor, const void* data,
                                          size_t size, bool reliable)>;

  MpActor(const LocationalData& locationalData_,
          const SubscribeCallback& onSubscribe_,
          const SubscribeCallback& onUnsubscribe_,
          const SendToUserFn& sendToUser_)
    : MpObjectReference(locationalData_, onSubscribe_, onUnsubscribe_,
                        nullBaseId, "ACHR")
    , sendToUser(sendToUser_)
  {
  }

  ~MpActor() = default;

  const auto& IsRaceMenuOpen() const { return isRaceMenuOpen; }
  auto GetLook() const { return look.get(); }
  const std::string& GetLookAsJson();
  const std::string& GetEquipmentAsJson() { return jEquipmentCache; };

  void SetRaceMenuOpen(bool isOpen);
  void SetLook(const Look* newLook);
  void SetEquipment(const std::string& jsonString);

  void SendToUser(const void* data, size_t size, bool reliable);

  class DestroyEventSink
  {
  public:
    virtual ~DestroyEventSink() = default;
    virtual void BeforeDestroy(MpActor& actor) = 0;
  };

  void AddEventSink(std::shared_ptr<DestroyEventSink> sink);
  void RemoveEventSink(std::shared_ptr<DestroyEventSink> sink);

private:
  void UnsubscribeFromAll();

  const SendToUserFn sendToUser;

  bool isRaceMenuOpen = false;
  std::unique_ptr<Look> look;
  std::string jLookCache;
  std::string jEquipmentCache;
  std::set<std::shared_ptr<DestroyEventSink>> destroyEventSinks;

protected:
  void BeforeDestroy() override;
};