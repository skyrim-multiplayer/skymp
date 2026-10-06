#include "PartOneOffloadSink.h"

#include "ActionListener.h"
#include "MpActor.h"
#include "PartOne.h"
#include "TeleportMessage2.h"

PartOneOffloadSink::PartOneOffloadSink(PartOne& partOne_)
  : partOne(partOne_)
{
}

MpActor* PartOneOffloadSink::ResolveActor(uint32_t formId) const
{
  // NoLoad on purpose: if the form is not resident any more there is nothing
  // to update, and triggering a lazy espm chunk load from the join phase
  // would be a surprising amount of work to do on behalf of a stale packet.
  const std::shared_ptr<MpForm>& form =
    partOne.worldState.LookupFormByIdNoLoad(formId);
  return form ? form->AsActor() : nullptr;
}

void PartOneOffloadSink::ApplyMovement(const MpParallel::ActorSnapshot& actor)
{
  MpActor* liveActor = ResolveActor(actor.formId);
  if (!liveActor) {
    ++staleActorCount;
    return;
  }

  partOne.GetActionListener().ApplyValidatedMovement(
    *liveActor,
    NiPoint3{ actor.proposedPos[0], actor.proposedPos[1],
              actor.proposedPos[2] },
    NiPoint3{ actor.proposedRot[0], actor.proposedRot[1],
              actor.proposedRot[2] },
    actor.isInJumpState, actor.isWeapDrawn, actor.isBlocking, actor.isSneaking,
    actor.isStanding, actor.idx);
}

void PartOneOffloadSink::SendCorrection(const MpParallel::ActorSnapshot& actor)
{
  // InvalidUserId here means the update targeted a hosted NPC rather than the
  // sender's own actor, which the inline path also leaves uncorrected.
  if (actor.ownerUserId == Networking::InvalidUserId) {
    return;
  }
  if (!partOne.IsConnected(actor.ownerUserId)) {
    return;
  }

  TeleportMessage2 msg;
  msg.pos = { actor.currentPos[0], actor.currentPos[1], actor.currentPos[2] };
  msg.rot = { actor.currentRot[0], actor.currentRot[1], actor.currentRot[2] };
  msg.worldOrCell = actor.currentWorldOrCell;
  partOne.GetSendTarget().Send(actor.ownerUserId, msg, true);
}

void PartOneOffloadSink::SendRelay(Networking::UserId userId,
                                   const uint8_t* data, size_t length,
                                   bool reliable)
{
  // The recipient list was resolved during ingest, so a player who
  // disconnected in between would otherwise be sent to here.
  if (!partOne.IsConnected(userId)) {
    return;
  }

  partOne.GetSendTarget().Send(
    userId, reinterpret_cast<Networking::PacketData>(data), length, reliable);
}
