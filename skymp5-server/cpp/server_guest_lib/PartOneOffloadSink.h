#pragma once
#include "parallel/OffloadDispatcher.h"

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
  void SendRelay(Networking::UserId userId, const uint8_t* data, size_t length,
                 bool reliable) override;

  // Counts submissions dropped at join time because the actor was gone.
  // Non-zero is expected during heavy churn; steadily growing is a bug.
  [[nodiscard]] uint64_t GetStaleActorCount() const noexcept
  {
    return staleActorCount;
  }

private:
  [[nodiscard]] MpActor* ResolveActor(uint32_t formId) const;

  PartOne& partOne;
  uint64_t staleActorCount = 0;
};
