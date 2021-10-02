import * as msg from "./messages";

export interface MsgHandler {
  createActor(msg: msg.CreateActorMessage): void;
  destroyActor(msg: msg.DestroyActorMessage): void;
  UpdateMovement(msg: msg.UpdateMovementMessage): void;
  UpdateAnimation(msg: msg.UpdateAnimationMessage): void;
  UpdateLook(msg: msg.UpdateLookMessage): void;
  UpdateEquipment(msg: msg.UpdateEquipmentMessage): void;
  setRaceMenuOpen(msg: msg.SetRaceMenuOpenMessage): void;
  customPacket(msg: msg.CustomPacket): void;

  handleConnectionAccepted(): void;
  handleDisconnect(): void;
}
