#pragma once
#include "Look.h"
#include "MpObjectReference.h"
#include <set>

class WorldState;

class MpActor : public MpObjectReference
{
public:
  static const char* Type() { return "Actor"; }
  const char* GetFormType() const override { return "Actor"; }

  MpActor(const LocationalData& locationalData_,
          const FormCallbacks& calbacks_, uint32_t optBaseId = 0);

  const bool& IsRaceMenuOpen() const;
  const Look* GetLook() const;
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
  void ApplyChangeForm(const MpChangeForm& changeForm) override;

private:
  void UnsubscribeFromAll();

  std::set<std::shared_ptr<DestroyEventSink>> destroyEventSinks;

  struct Impl;
  std::shared_ptr<Impl> pImpl;

protected:
  void BeforeDestroy() override;
};