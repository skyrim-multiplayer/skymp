#include "MpActor.h"
#include "WorldState.h"
#include <NiPoint3.h>

void MpActor::SetRaceMenuOpen(bool isOpen)
{
  isRaceMenuOpen = isOpen;
}

void MpActor::SetLook(const Look* newLook)
{
  jLookCache.clear();
  if (newLook) {
    look.reset(new Look(*newLook));
  } else {
    look.reset();
  }
}

void MpActor::SetEquipment(const std::string& jsonString)
{
  jEquipmentCache = jsonString;
}

void MpActor::SendToUser(const void* data, size_t size, bool reliable)
{
  if (sendToUser)
    sendToUser(this, data, size, reliable);
  else
    throw std::runtime_error("sendToUser is nullptr");
}

void MpActor::AddEventSink(std::shared_ptr<DestroyEventSink> sink)
{
  destroyEventSinks.insert(sink);
}

void MpActor::RemoveEventSink(std::shared_ptr<DestroyEventSink> sink)
{
  destroyEventSinks.erase(sink);
}

const std::string& MpActor::GetLookAsJson()
{
  if (look && jLookCache.empty())
    jLookCache = look->ToJson();
  return jLookCache;
}

void MpActor::UnsubscribeFromAll()
{
  auto emittersCopy = GetEmitters();
  for (auto emitter : emittersCopy)
    if (emitter != this)
      Unsubscribe(emitter, this);
}

void MpActor::BeforeDestroy()
{
  for (auto& sink : destroyEventSinks)
    sink->BeforeDestroy(*this);

  MpObjectReference::BeforeDestroy();

  UnsubscribeFromAll();
}