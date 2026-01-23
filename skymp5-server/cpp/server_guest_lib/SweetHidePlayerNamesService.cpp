#include "SweetHidePlayerNamesService.h"
#include "MpActor.h"
#include "PartOne.h"

SweetHidePlayerNamesService::SweetHidePlayerNamesService(PartOne& partOne_)
  : partOne(partOne_)
{
  partOne.SetOnActorStreamIn([this](const MpActor& emitter,
                                    const MpObjectReference& listener,
                                    CreateActorMessage& message) {
    this->OnActorStreamIn(emitter, listener, message);
  });
}

void SweetHidePlayerNamesService::OnActorStreamIn(
  const MpActor& emitter, const MpObjectReference& listener,
  CreateActorMessage& message)
{
  if (message.appearance.has_value() && &emitter != &listener) {
    message.appearance->name = "Stranger";
  }
}
