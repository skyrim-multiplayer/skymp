#pragma once
#include "ParallelConfig.h"
#include "RelayPlan.h"
#include "TickSnapshot.h"
#include <cstdint>

namespace MpParallel {

// Maximum squared displacement a single movement update may claim. Matches
// kSqrMaxDistance in MovementValidation.cpp; the two have to stay in step or
// the offloaded path would accept updates the inline path rejects.
constexpr float kSqrMaxMovementDistance = 4096.f * 4096.f;

// The pure half of the framework: given a snapshot, decide what should
// happen. Nothing here touches the world, the network, or any shared mutable
// state, which is exactly why it can run on a worker thread.
namespace InterestManager {

// Reproduces MovementValidation::Validate against snapshot data.
//
// Returns true when the proposed transform is acceptable. A set teleportFlag
// always fails, because the inline path substitutes an infinite position in
// that case to force the client back to the server's transform.
[[nodiscard]] bool ValidateMovement(const ActorSnapshot& actor) noexcept;

// How many ticks apart relays to this recipient should be spaced.
//
// 1 means "every tick". The value grows with distance and with how far the
// cluster is over its time budget, so a quiet server never throttles and a
// packed city square degrades smoothly instead of stalling the tick.
[[nodiscard]] uint32_t ComputeSkipFactor(float sqrDistance,
                                         uint32_t pressureLevel,
                                         const ParallelConfig& config) noexcept;

// Whether this particular (sender, recipient) edge transmits on this tick.
//
// The phase is derived from the pair rather than shared, so a throttled
// cluster spreads its relays evenly over the skip window instead of sending
// everything on the same tick and idling in between.
[[nodiscard]] bool ShouldRelayThisTick(uint64_t tickIndex, uint32_t senderFormId,
                                       Networking::UserId recipientUserId,
                                       uint32_t skipFactor) noexcept;

// Processes a contiguous range of a cluster's members: validates each one's
// update and expands its relay list into concrete sends. `output` is reset
// before use.
//
// The unit of work is a range rather than a whole cluster because a single
// crowded area is routinely most of a tick's work, and a design that could
// not split it would cap the achievable speedup at whatever fraction the
// other areas contribute. Each sender's work depends only on the immutable
// snapshot, so any partition of the members is safe; keeping the ranges
// contiguous and processing them in order is what keeps the join
// deterministic.
//
// pressureLevel is derived from the previous tick's measurement, so it is a
// plain input here and introduces no cross-task dependency.
void ProcessRange(const TickSnapshot& snapshot, const uint32_t* actorIndices,
                  size_t actorCount, uint32_t pressureLevel,
                  const ParallelConfig& config, ClusterOutput& output);

}

}
