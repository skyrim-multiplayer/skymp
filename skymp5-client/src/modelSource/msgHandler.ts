import * as msg from "../messages";
import { ChangeValuesMessage } from "../services/messages/changeValues";
import { UpdateAnimationMessage } from "../services/messages/updateAnimationMessage";
import { UpdateAppearanceMessage } from "../services/messages/updateAppearanceMessage";
import { UpdateEquipmentMessage } from "../services/messages/updateEquipmentMessage";
import { UpdateMovementMessage } from "../services/messages/updateMovementMessage";

export interface MsgHandler {
  createActor(msg: msg.CreateActorMessage): void;
  destroyActor(msg: msg.DestroyActorMessage): void;
  UpdateMovement(msg: UpdateMovementMessage): void;
  UpdateAnimation(msg: UpdateAnimationMessage): void;
  UpdateAppearance(msg: UpdateAppearanceMessage): void;
  UpdateEquipment(msg: UpdateEquipmentMessage): void;
  ChangeValues(msg: ChangeValuesMessage): void;
  setRaceMenuOpen(msg: msg.SetRaceMenuOpenMessage): void;
  customPacket(msg: msg.CustomPacket): void;
  DeathStateContainer(msg: msg.DeathStateContainerMessage): void;

  handleConnectionAccepted(): void;
  handleDisconnect(): void;
}
