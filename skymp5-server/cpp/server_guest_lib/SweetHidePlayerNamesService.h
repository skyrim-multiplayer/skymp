#pragma once
class PartOne;
class MpActor;
class MpObjectReference;
struct CreateActorMessage;

class SweetHidePlayerNamesService
{
public:
  explicit SweetHidePlayerNamesService(PartOne& partOne);

private:
  void OnActorStreamIn(const MpActor& emitter,
                       const MpObjectReference& listener,
                       CreateActorMessage& message);

  PartOne& partOne;
};
