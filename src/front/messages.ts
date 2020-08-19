import { Movement, Transform } from "../lib/structures/movement";
import { Look } from "../lib/structures/look";
import { Animation } from "../lib/structures/animation";
import { Equipment } from "../lib/structures/equipment";
import { Inventory } from "../lib/structures/inventory";

export enum MsgType {
  CustomPacket = 1,
  UpdateMovement = 2,
  UpdateAnimation = 3,
  UpdateLook = 4,
  UpdateEquipment = 5,
  Activate = 6,
  UpdateProperty = 7,
  PutItem = 8,
  TakeItem = 9,
}

export interface SetInventory {
  type: "setInventory";
  inventory: Inventory;
}

export interface OpenContainer {
  type: "openContainer";
  target: number;
}

export interface Teleport {
  type: "teleport";
  pos: number[];
  rot: number[];
  worldOrCell: number;
}

export interface CreateActorMessage {
  type: "createActor";
  idx: number;
  refrId?: number;
  transform: Transform;
  isMe: boolean;
  look?: Look;
  equipment?: Equipment;
  inventory?: Inventory;
  baseId?: number;
  props?: Record<string, unknown>;
}

export interface DestroyActorMessage {
  type: "destroyActor";
  idx: number;
}

export interface UpdateMovementMessage {
  t: MsgType.UpdateMovement;
  idx: number;
  data: Movement;
}

export interface UpdateAnimationMessage {
  t: MsgType.UpdateAnimation;
  idx: number;
  data: Animation;
}

export interface UpdateLookMessage {
  t: MsgType.UpdateLook;
  idx: number;
  data: Look;
}

export interface UpdateEquipmentMessage {
  t: MsgType.UpdateEquipment;
  idx: number;
  data: Equipment;
}

export interface UpdatePropertyMessage {
  t: MsgType.UpdateProperty;
  idx: number;
  data: unknown;
  propName: string;
}

export interface SetRaceMenuOpenMessage {
  type: "setRaceMenuOpen";
  open: boolean;
}

export interface CustomPacket {
  type: "customPacket";
  content: Record<string, unknown>;
}
