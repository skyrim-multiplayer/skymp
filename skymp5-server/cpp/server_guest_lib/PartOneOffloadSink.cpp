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

void PartOneOffloadSink::BeginJoin()
{
  const size_t maxUserId = partOne.serverState.maxConnectedId;
  connectedBits.assign(maxUserId / 64 + 1, 0);

  for (size_t i = 0; i <= maxUserId; ++i) {
    const auto userId = static_cast<Networking::UserId>(i);
    if (partOne.IsConnected(userId)) {
      connectedBits[i / 64] |= 1ULL << (i % 64);
    }
  }
}

void PartOneOffloadSink::SendRelayBatch(const MpParallel::OutboundSend* sends,
                                        size_t count,
                                        const uint8_t* packetBytes,
                                        size_t packetBytesLength)
{
  if (sends == nullptr || packetBytes == nullptr) {
    return;
  }

  // Both hoisted out of the loop. GetSendTarget in particular is a pointer
  // chase and a throw-if-null, and this loop is the N^2 term of the whole
  // server.
  PartOneSendTargetWrapper& sendTarget = partOne.GetSendTarget();

  for (size_t i = 0; i < count; ++i) {
    const MpParallel::OutboundSend& send = sends[i];

    if (send.byteLength == 0 ||
        static_cast<size_t>(send.byteOffset) + send.byteLength >
          packetBytesLength) {
      continue;
    }

    // The recipient list is snapshotted at the top of the tick, so a player
    // who left since then would otherwise be sent to here.
    if (!IsConnectedFast(send.userId)) {
      continue;
    }

    sendTarget.Send(
      send.userId,
      reinterpret_cast<Networking::PacketData>(packetBytes + send.byteOffset),
      send.byteLength, send.reliable);
  }
}
