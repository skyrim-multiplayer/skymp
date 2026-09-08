#include "InterestManager.h"

#include <algorithm>
#include <limits>

namespace MpParallel {
namespace InterestManager {

namespace {

constexpr int32_t kInt16Min = std::numeric_limits<int16_t>::min();
constexpr int32_t kInt16Max = std::numeric_limits<int16_t>::max();

[[nodiscard]] float SqrDistance(const float a[3], const float b[3]) noexcept
{
  const float dx = a[0] - b[0];
  const float dy = a[1] - b[1];
  const float dz = a[2] - b[2];
  return dx * dx + dy * dy + dz * dz;
}

// splitmix64-style mix of the pair identity. Only needs to spread phases
// evenly across the skip window, so a cheap finalizer is enough.
[[nodiscard]] uint64_t MixPair(uint32_t senderFormId,
                               Networking::UserId recipientUserId) noexcept
{
  uint64_t h = (static_cast<uint64_t>(senderFormId) << 16) |
    static_cast<uint64_t>(recipientUserId);
  h ^= h >> 30;
  h *= 0xbf58476d1ce4e5b9ULL;
  h ^= h >> 27;
  h *= 0x94d049bb133111ebULL;
  h ^= h >> 31;
  return h;
}

// Maps a hash uniformly onto [0, range) without a division.
//
// Lemire's multiply-shift reduction. The distribution is not perfectly
// uniform -- some outputs are one 2^-32 slice more likely than others -- but
// this only picks which tick of a 2-to-4-tick window a pair transmits on, and
// the bias is far below anything observable there.
[[nodiscard]] uint32_t ReduceToRange(uint64_t hash, uint32_t range) noexcept
{
  const uint64_t product =
    static_cast<uint64_t>(static_cast<uint32_t>(hash >> 32)) * range;
  return static_cast<uint32_t>(product >> 32);
}

}

InterestPolicy InterestPolicy::Build(const ParallelConfig& config,
                                     uint64_t tickIndex) noexcept
{
  InterestPolicy policy;
  policy.interestManagement = config.interestManagement;
  policy.adaptiveThrottling = config.adaptiveThrottling;

  const float full = config.interestFullRateUnits;
  policy.sqrFull = full * full;
  policy.sqrFull4 = policy.sqrFull * 4.f;
  policy.sqrFull16 = policy.sqrFull * 16.f;

  const float throttle = config.throttleDistanceUnits;
  policy.sqrThrottle = throttle * throttle;
  policy.sqrThrottle4 = policy.sqrThrottle * 4.f;
  policy.sqrThrottle16 = policy.sqrThrottle * 16.f;

  policy.maxInterestSkip =
    std::min(std::max<uint32_t>(config.maxInterestSkipTicks, 1), kMaxSkipFactor);
  policy.maxThrottleSkip =
    std::min(std::max<uint32_t>(config.maxThrottleSkipTicks, 1), kMaxSkipFactor);

  // The whole point of the table: this is kMaxSkipFactor divisions once per
  // tick instead of one per relay edge.
  policy.tickPhase[0] = 0;
  for (uint32_t f = 1; f <= kMaxSkipFactor; ++f) {
    policy.tickPhase[f] = static_cast<uint32_t>(tickIndex % f);
  }
  return policy;
}

uint32_t InterestPolicy::SkipFactor(float sqrDistance,
                                    uint32_t pressureLevel) const noexcept
{
  // Always-on, load-independent rate reduction by distance.
  //
  // Relay volume is the quadratic term, and emitting the sends is serial no
  // matter how the decisions were parallelised, so sending fewer of them is
  // the only thing that attacks the real cost. Nearby recipients are never
  // affected.
  uint32_t interest = 1;
  if (interestManagement && sqrDistance > sqrFull) {
    // One step per doubling of the full-rate radius. Squared comparisons keep
    // this free of square roots on the hottest path in the server.
    interest = 2;
    if (sqrDistance > sqrFull4) {
      interest = 3;
    }
    if (sqrDistance > sqrFull16) {
      interest = 4;
    }
    interest = std::min(interest, maxInterestSkip);
  }

  if (!adaptiveThrottling || pressureLevel == 0) {
    return interest;
  }

  // Inside the near band nothing is held back by pressure: those are the
  // players actually fighting or talking to each other, and they are what
  // "smooth" means to a user.
  //
  // The band shrinks as pressure rises, and it has to. Held fixed at
  // throttleDistanceUnits -- 4096, one whole chunk -- the exemption covered
  // every pair in a crowd that had packed into a single chunk, which is the
  // only situation the throttle exists for. Measured on 300 players walking
  // into one square, the mechanism suppressed exactly zero edges at every
  // budget tried, from 8000us down to 250us; dropping the radius to 400 units
  // took it from 0 to 21,000. It was not that the server was never judged
  // under pressure, it was that nobody was ever far enough away to act on.
  //
  // Halving the radius per pressure level keeps the guarantee where it counts
  // -- at the highest level the exemption is still 512 units, well inside a
  // fight -- while letting the throttle reach a dense crowd at all.
  const uint32_t shift = std::min<uint32_t>(pressureLevel, 3);
  const float bandScale = 1.f / static_cast<float>(1u << (2u * shift));
  const float nearBand = sqrThrottle * bandScale;

  if (sqrDistance <= nearBand) {
    return interest;
  }

  // One extra step per doubling of the (scaled) threshold distance.
  uint32_t distanceBand = 1;
  if (sqrDistance > sqrThrottle4 * bandScale) {
    distanceBand = 2;
  }
  if (sqrDistance > sqrThrottle16 * bandScale) {
    distanceBand = 3;
  }

  const uint64_t factor =
    static_cast<uint64_t>(pressureLevel) * distanceBand + 1;
  const auto pressureSkip =
    static_cast<uint32_t>(std::min<uint64_t>(factor, maxThrottleSkip));

  // The two mechanisms stack by taking the stronger one rather than
  // multiplying, so an overloaded server never starves a distant player
  // beyond whichever cap is the more conservative.
  return std::max(interest, pressureSkip);
}

bool InterestPolicy::Transmits(uint32_t skipFactor, uint32_t senderFormId,
                               Networking::UserId recipientUserId) const noexcept
{
  if (skipFactor <= 1) {
    return true;
  }
  const uint32_t phase =
    ReduceToRange(MixPair(senderFormId, recipientUserId), skipFactor);
  return tickPhase[skipFactor] == phase;
}

bool ValidateMovement(const ActorSnapshot& actor) noexcept
{
  // The inline path substitutes an infinite target position when the
  // teleport flag is set, which makes the distance test below fail. Encoding
  // that directly is clearer and avoids relying on infinity arithmetic.
  if (actor.teleportFlag) {
    return false;
  }

  if (actor.currentWorldOrCell != actor.proposedWorldOrCell) {
    return false;
  }

  return SqrDistance(actor.currentPos, actor.proposedPos) <
    kSqrMaxMovementDistance;
}

uint32_t ComputeSkipFactor(float sqrDistance, uint32_t pressureLevel,
                           const ParallelConfig& config) noexcept
{
  // tickIndex is irrelevant to the factor itself, only to the phase.
  return InterestPolicy::Build(config, 0).SkipFactor(sqrDistance,
                                                     pressureLevel);
}

bool ShouldRelayThisTick(uint64_t tickIndex, uint32_t senderFormId,
                         Networking::UserId recipientUserId,
                         uint32_t skipFactor) noexcept
{
  if (skipFactor <= 1) {
    return true;
  }
  if (skipFactor > kMaxSkipFactor) {
    skipFactor = kMaxSkipFactor;
  }
  const uint32_t phase =
    ReduceToRange(MixPair(senderFormId, recipientUserId), skipFactor);
  return static_cast<uint32_t>(tickIndex % skipFactor) == phase;
}

void ProcessRange(const TickSnapshot& snapshot, const uint32_t* actorIndices,
                  size_t actorCount, uint32_t pressureLevel,
                  const InterestPolicy& policy, ClusterOutput& output)
{
  output.Reset();
  if (actorIndices == nullptr || actorCount == 0) {
    return;
  }
  output.verdicts.reserve(actorCount);

  const RelayChunk* const chunksBegin = snapshot.relayChunks.data();
  const RelayChunk* const chunksEnd = chunksBegin + snapshot.relayChunks.size();
  const RelayTarget* const targets = snapshot.relayTargets.data();

  const auto chunkLess = [](const RelayChunk& chunk,
                            const AreaKey& key) noexcept {
    return chunk.key < key;
  };

  for (size_t i = 0; i < actorCount; ++i) {
    const uint32_t actorIndex = actorIndices[i];
    if (actorIndex >= snapshot.actors.size()) {
      continue;
    }
    const ActorSnapshot& actor = snapshot.actors[actorIndex];

    const bool accepted = ValidateMovement(actor);

    MovementVerdict verdict;
    verdict.actorIndex = actorIndex;
    verdict.accepted = accepted;
    // Only the owning player gets pulled back; a hosted NPC has no client to
    // correct, matching the `isMe` guard in MovementValidation.
    verdict.needsCorrection =
      !accepted && actor.ownerUserId != Networking::InvalidUserId;
    output.verdicts.push_back(verdict);

    // The relay happens before validation on the inline path, so a rejected
    // update is still forwarded to neighbours. Preserving that ordering
    // keeps observable behaviour identical.
    if (actor.packetLength == 0) {
      continue;
    }

    const int32_t centreX = actor.area.chunkX;
    const int32_t centreY = actor.area.chunkY;

    // The recipients are whoever occupies the 3x3 stencil around the sender,
    // which is Grid.h's visibility neighbourhood. One binary search per
    // column rather than one per cell: relayChunks is ordered by
    // (world, x, y), so a column's three rows are adjacent in it.
    for (int32_t dx = -1; dx <= 1; ++dx) {
      const int32_t nx = centreX + dx;
      if (nx < kInt16Min || nx > kInt16Max) {
        continue;
      }

      const int32_t loY = std::max(centreY - 1, kInt16Min);
      const int32_t hiY = std::min(centreY + 1, kInt16Max);

      const AreaKey lo{ actor.worldOrCell, static_cast<int16_t>(nx),
                        static_cast<int16_t>(loY) };
      const AreaKey hi{ actor.worldOrCell, static_cast<int16_t>(nx),
                        static_cast<int16_t>(hiY) };

      for (const RelayChunk* chunk =
             std::lower_bound(chunksBegin, chunksEnd, lo, chunkLess);
           chunk != chunksEnd && !(hi < chunk->key); ++chunk) {

        for (uint32_t t = chunk->begin; t < chunk->end; ++t) {
          const RelayTarget& target = targets[t];
          if (target.userId == Networking::InvalidUserId) {
            continue;
          }

          const float sqrDistance = SqrDistance(actor.currentPos, target.pos);
          const uint32_t skipFactor =
            policy.SkipFactor(sqrDistance, pressureLevel);

          if (!policy.Transmits(skipFactor, actor.formId, target.userId)) {
            ++output.throttledEdges;
            continue;
          }

          OutboundSend send;
          send.userId = target.userId;
          send.byteOffset = actor.packetOffset;
          send.byteLength = actor.packetLength;
          // Movement relays are unreliable on the inline path too: the next
          // update supersedes this one, so a retransmit would only add
          // latency.
          send.reliable = false;
          output.sends.push_back(send);
          ++output.emittedEdges;
        }
      }
    }
  }
}

}
}
