import * as msg from '../messages';

export interface MsgHandler {
  createActor(msg: msg.CreateActorMessage): void;
  destroyActor(msg: msg.DestroyActorMessage): void;
  UpdateMovement(msg: msg.UpdateMovementMessage): void;
  UpdateAnimation(msg: msg.UpdateAnimationMessage): void;
  UpdateAppearance(msg: msg.UpdateAppearanceMessage): void;
  UpdateEquipment(msg: msg.UpdateEquipmentMessage): void;
  ChangeValues(msg: msg.ChangeValuesMessage): void;
  setRaceMenuOpen(msg: msg.SetRaceMenuOpenMessage): void;
  customPacket(msg: msg.CustomPacket): void;
  DeathStateContainer(msg: msg.DeathStateContainerMessage): void;

  handleConnectionAccepted(): void;
  handleDisconnect(): void;
}
