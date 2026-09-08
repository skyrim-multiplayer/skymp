#pragma once
#include "AreaKey.h"
#include <NetworkingInterface.h>
#include <cstdint>
#include <vector>

namespace MpParallel {

constexpr uint32_t kUnassignedCluster = static_cast<uint32_t>(-1);

// One actor whose owner sent a movement update this tick.
//
// Everything a worker needs is copied in here so that no worker ever
// dereferences an MpActor, a WorldState, or the espm loader. That is the
// single invariant the whole framework rests on: the parallel phase reads
// only trivially-copyable data that the main thread has stopped writing.
struct ActorSnapshot
{
  uint32_t formId = 0;
  uint32_t idx = 0;

  // Owner of this actor, or InvalidUserId for a hosted NPC.
  Networking::UserId ownerUserId = Networking::InvalidUserId;

  uint32_t worldOrCell = 0;

  // Position and rotation the client is claiming, i.e. the candidate that
  // still has to pass validation.
  float proposedPos[3] = { 0.f, 0.f, 0.f };
  float proposedRot[3] = { 0.f, 0.f, 0.f };
  uint32_t proposedWorldOrCell = 0;

  // Server-authoritative state as of the start of the tick, used both for
  // validation and as the correction payload when validation fails.
  float currentPos[3] = { 0.f, 0.f, 0.f };
  float currentRot[3] = { 0.f, 0.f, 0.f };
  uint32_t currentWorldOrCell = 0;

  // Set when the client asked to be teleported, which suppresses the
  // distance check exactly as the inline path does.
  bool teleportFlag = false;

  // Animation graph state carried by the update. Applied by the join phase
  // only when validation passed, matching the inline ordering. runMode is
  // reduced to the single predicate the handler actually tests.
  bool isInJumpState = false;
  bool isWeapDrawn = false;
  bool isBlocking = false;
  bool isSneaking = false;
  bool isStanding = false;

  // Chunk of `currentPos`, i.e. the chunk this actor is partitioned by.
  AreaKey area;

  // Half-open range into TickSnapshot::rawPacketBytes holding the packet that
  // has to be relayed verbatim to the recipients.
  uint32_t packetOffset = 0;
  uint32_t packetLength = 0;

  // Index assigned by the partitioner. kUnassignedCluster until then.
  uint32_t clusterIndex = kUnassignedCluster;
};

// A single actor that is connected and active. The main thread snapshots
// every active player once per tick, and workers spatial-filter this list.
struct RelayTarget
{
  Networking::UserId userId = Networking::InvalidUserId;
  uint32_t listenerFormId = 0;
  float pos[3] = { 0.f, 0.f, 0.f };
  uint32_t worldOrCell = 0;
  int16_t chunkX = 0;
  int16_t chunkY = 0;
};

// Immutable-during-the-parallel-phase view of everything submitted this tick.
//
// The main thread fills this during ingest and join; workers only read it.
struct TickSnapshot
{
  std::vector<ActorSnapshot> actors;
  std::vector<RelayTarget> relayTargets;

  // Raw packet payloads, concatenated. Relaying forwards these bytes
  // untouched, exactly as ActionListener::SendToNeighbours does, so the wire
  // format is unchanged.
  std::vector<uint8_t> rawPacketBytes;

  // Monotonic tick counter, used by the throttle to decide whose turn it is.
  uint64_t tickIndex = 0;

  void Clear() noexcept
  {
    actors.clear();
    relayTargets.clear();
    rawPacketBytes.clear();
  }

  [[nodiscard]] bool Empty() const noexcept { return actors.empty(); }
};

}
