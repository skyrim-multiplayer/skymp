#pragma once
#include <NetworkingInterface.h>
#include <cstdint>
#include <vector>

namespace MpParallel {

// One packet the join phase has to hand to the send target. The payload is a
// range inside TickSnapshot::rawPacketBytes rather than a copy, because the
// relay forwards the client's packet verbatim and many recipients share the
// same bytes.
struct OutboundSend
{
  Networking::UserId userId = Networking::InvalidUserId;
  uint32_t byteOffset = 0;
  uint32_t byteLength = 0;
  bool reliable = false;
};

// What the parallel phase decided about one actor's movement update.
struct MovementVerdict
{
  uint32_t actorIndex = 0;

  // False means the update failed validation: the join phase must skip the
  // state write and send the owner a correction instead.
  bool accepted = false;

  // Set when the rejection should produce a TeleportMessage2. Mirrors the
  // `isMe` condition of the inline path: hosted NPCs are not corrected.
  bool needsCorrection = false;
};

// Per-cluster results. Exactly one task writes each instance, and the join
// phase reads them back in cluster order, which is what keeps the outcome
// independent of thread scheduling.
//
// Cache-line aligned because the outer vector puts these next to each other
// and different threads append to neighbouring entries throughout the
// parallel phase.
#if defined(_MSC_VER)
#  pragma warning(push)
// C4324: padded because of the alignment specifier. That is the entire point
// here, and the build treats warnings as errors.
#  pragma warning(disable : 4324)
#endif
struct alignas(64) ClusterOutput
{
  std::vector<OutboundSend> sends;
  std::vector<MovementVerdict> verdicts;

  // Relay edges suppressed by the distance/rate policy this tick.
  uint64_t throttledEdges = 0;

  // Relay edges emitted.
  uint64_t emittedEdges = 0;

  // Wall-clock cost of the task that produced this output, in microseconds.
  // Feeds the load balancer's estimate for the next tick.
  uint64_t elapsedMicros = 0;

  void Reset() noexcept
  {
    // clear() keeps the capacity, so a steady-state tick does not allocate.
    sends.clear();
    verdicts.clear();
    throttledEdges = 0;
    emittedEdges = 0;
    elapsedMicros = 0;
  }
};
#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

}
