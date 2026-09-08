#pragma once
#include "parallel/OffloadDispatcher.h"
#include <cstdint>
#include <vector>

class PartOne;
class MpActor;

// Applies the parallel phase's decisions to the live world.
//
// Everything here runs on the main thread during the join, after the workers
// have stopped. The one thing it must be careful about is time: a submission
// is made while packets are still being pumped, and by the time the join runs
// the actor may have been destroyed, or its owner may have disconnected. Each
// method therefore re-resolves by form id and checks the connection before
// touching anything.
class PartOneOffloadSink : public MpParallel::IOffloadSink
{
public:
  explicit PartOneOffloadSink(PartOne& partOne_);

  void ApplyMovement(const MpParallel::ActorSnapshot& actor) override;
  void SendCorrection(const MpParallel::ActorSnapshot& actor) override;
  void SendRelayBatch(const MpParallel::OutboundSend* sends, size_t count,
                      const uint8_t* packetBytes,
                      size_t packetBytesLength) override;

  // Snapshots which users are connected, once, before the join walks the
  // relay lists.
  //
  // Relaying checks the recipient is still connected, and doing that through
  // ServerState is two dependent loads into a vector of unique_ptr. That is
  // nothing on its own and a real cost at one per relay edge, so the answer
  // is precomputed into a bitmap the join can test with a shift and a mask.
  void BeginJoin();

  // Counts submissions dropped at join time because the actor was gone.
  // Non-zero is expected during heavy churn; steadily growing is a bug.
  [[nodiscard]] uint64_t GetStaleActorCount() const noexcept
  {
    return staleActorCount;
  }

private:
  [[nodiscard]] MpActor* ResolveActor(uint32_t formId) const;

  [[nodiscard]] bool IsConnectedFast(Networking::UserId userId) const noexcept
  {
    const size_t word = static_cast<size_t>(userId) / 64;
    if (word >= connectedBits.size()) {
      return false;
    }
    return (connectedBits[word] >> (static_cast<size_t>(userId) % 64)) & 1ULL;
  }

  PartOne& partOne;
  uint64_t staleActorCount = 0;

  // One bit per user id, rebuilt by BeginJoin.
  std::vector<uint64_t> connectedBits;
};
