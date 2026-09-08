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

// Largest skip factor either mechanism may produce. Normalize clamps both
// maxInterestSkipTicks and maxThrottleSkipTicks to this, which is what lets
// InterestPolicy precompute a phase table instead of dividing per edge.
constexpr uint32_t kMaxSkipFactor = 32;

// The pure half of the framework: given a snapshot, decide what should
// happen. Nothing here touches the world, the network, or any shared mutable
// state, which is exactly why it can run on a worker thread.
namespace InterestManager {

// Everything the per-edge decision needs, with the per-tick and per-config
// invariants already folded in.
//
// This exists because the decision runs once per (sender, recipient) pair and
// that is the N^2 term: at 400 players in one area it is 160,000 evaluations
// a tick. Squaring the configured radii and reducing the tick index modulo
// every possible skip factor are loop invariants, and the modulo in
// particular is an integer division -- tens of cycles -- that used to be paid
// twice per throttled edge.
struct InterestPolicy
{
  bool interestManagement = false;
  bool adaptiveThrottling = false;

  // Squared interest radius and its two doubling steps.
  float sqrFull = 0.f;
  float sqrFull4 = 0.f;
  float sqrFull16 = 0.f;

  // Squared pressure-throttle radius and its two doubling steps.
  float sqrThrottle = 0.f;
  float sqrThrottle4 = 0.f;
  float sqrThrottle16 = 0.f;

  uint32_t maxInterestSkip = 1;
  uint32_t maxThrottleSkip = 1;

  // tickPhase[f] is tickIndex % f, for every f a skip factor can take.
  // Index 0 is unused and index 1 is always 0.
  uint32_t tickPhase[kMaxSkipFactor + 1] = {};

  [[nodiscard]] static InterestPolicy Build(const ParallelConfig& config,
                                            uint64_t tickIndex) noexcept;

  // How many ticks apart relays to a recipient at this distance should be
  // spaced. 1 means "every tick".
  [[nodiscard]] uint32_t SkipFactor(float sqrDistance,
                                    uint32_t pressureLevel) const noexcept;

  // Whether this particular (sender, recipient) edge transmits on this tick.
  // `skipFactor` must be in [1, kMaxSkipFactor].
  [[nodiscard]] bool Transmits(uint32_t skipFactor, uint32_t senderFormId,
                               Networking::UserId recipientUserId) const
    noexcept;
};

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
//
// Equivalent to InterestPolicy::SkipFactor; kept as a standalone entry point
// because it is the readable statement of the policy and what the tests
// pin down.
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
                  const InterestPolicy& policy, ClusterOutput& output);

}

}
