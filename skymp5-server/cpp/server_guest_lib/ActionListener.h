#pragma once
#include "AnimationData.h"
#include "ConsoleCommands.h"
#include "CraftService.h"
#include "HitData.h"
#include "Messages.h"
#include "MessageEvent.h"
#include "MpActor.h"
#include "RawMessageData.h"
#include "ServiceBase.h"
#include "SpellCastData.h"
#include "libespm/Loader.h"
#include <memory>

class PartOne;
class ServerState;
class WorldState;
struct ActorValues;

class ActionListener : public ServiceBase<ActionListener>
{
public:
  explicit ActionListener(PartOne& partOne_);

  void OnCustomPacket(const MessageEvent<CustomPacketMessage>& event);
  void OnUpdateMovement(const MessageEvent<UpdateMovementMessage>& event);
  void OnUpdateAnimation(const MessageEvent<UpdateAnimationMessage>& event);
  void OnUpdateAppearance(const MessageEvent<UpdateAppearanceMessage>& event);
  void OnUpdateEquipment(const MessageEvent<UpdateEquipmentMessage>& event);
  void OnActivate(const MessageEvent<ActivateMessage>& event);
  void OnPutItem(const MessageEvent<PutItemMessage>& event);
  void OnTakeItem(const MessageEvent<TakeItemMessage>& event);
  void OnDropItem(const MessageEvent<DropItemMessage>& event);
  void OnPlayerBowShot(const MessageEvent<PlayerBowShotMessage>& event);
  void OnFinishSpSnippet(const MessageEvent<FinishSpSnippetMessage>& event);
  void OnEquip(const MessageEvent<OnEquipMessage>& event);
  void OnConsoleCommand(const MessageEvent<ConsoleCommandMessage>& event);
  void OnCraftItem(const MessageEvent<CraftItemMessage>& event);
  void OnHostAttempt(const MessageEvent<HostMessage>& event);
  void OnCustomEvent(const MessageEvent<CustomEventMessage>& event);
  void OnChangeValues(const MessageEvent<ChangeValuesMessage>& event);
  void OnHit(const MessageEvent<HitMessage>& event);
  void OnUpdateAnimVariables(const MessageEvent<UpdateAnimVariablesMessage>& event);
  void OnSpellCast(const MessageEvent<SpellCastMessage>& event);
  void OnUnknown(const RawMessageData& rawMsgData);

private:
  void OnSpellHit(MpActor* aggressor, MpObjectReference* targetRef,
                  const HitData& hitData);
  void OnWeaponHit(MpActor* aggressor, MpObjectReference* targetRef,
                   HitData hitData, bool isUnarmed);

  void SendPapyrusOnHitEvent(MpActor* aggressor, MpObjectReference* target,
                             const HitData& hitData);

  // Returns user's actor if there is attached one
  MpActor* SendToNeighbours(uint32_t idx, Networking::UserId userId,
                            Networking::PacketData data, size_t length,
                            bool reliable);

  MpActor* SendToNeighbours(uint32_t idx, const RawMessageData& rawMsgData,
                            bool reliable = false);

  PartOne& partOne;
};
