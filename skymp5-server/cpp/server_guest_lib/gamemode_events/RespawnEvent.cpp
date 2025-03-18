#include "RespawnEvent.h"

#include "MpActor.h"

RespawnEvent::RespawnEvent(MpActor* actor_, bool shouldTeleport_)
  : actor(actor_)
  , shouldTeleport(shouldTeleport_)
{
}

const char* RespawnEvent::GetName() const
{
  return "onRespawn";
}

std::string RespawnEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(actor->GetFormId());
  result += "]";
  return result;
}

void RespawnEvent::OnFireSuccess(WorldState*)
{
  actor->SendAndSetDeathState(false, shouldTeleport);

  // TODO: should probably not sending to ourselves. see also RespawnTest.cpp
  actor->SendMessageToActorListeners(
    actor->CreatePropertyMessage(actor, "isDead", /*value=*/false),
    /*reliable=*/true);
}
