#pragma once
#include "Look.h"
#include "MpObjectReference.h"
#include <set>

class WorldState;

class MpActor : public MpObjectReference
{
public:
  static const char* Type() { return "Actor"; }

  constexpr static uint32_t nullBaseId = 0;

  using SendToUserFn = std::function<void(MpActor* actor, const void* data,
                                          size_t size, bool reliable)>;

  MpActor(const LocationalData& locationalData_,
          const SubscribeCallback& onSubscribe_,
          const SubscribeCallback& onUnsubscribe_,
          const SendToUserFn& sendToUser_);

  const bool& IsRaceMenuOpen() const;
  Look* GetLook() const;
  std::string GetLookAsJson();
  std::string GetEquipmentAsJson();

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

  MpChangeForm GetChangeForm() const override;

private:
  void UnsubscribeFromAll();

  const SendToUserFn sendToUser;
  std::set<std::shared_ptr<DestroyEventSink>> destroyEventSinks;

  struct Impl;
  std::shared_ptr<Impl> pImpl;

protected:
  void BeforeDestroy() override;
};