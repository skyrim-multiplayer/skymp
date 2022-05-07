import { Form } from './form';
import { Inventory } from './mp';

export class ObjectReference extends Form {
  constructor(formId: number) {}
  Activate(activator: ObjectReference);
  AddItem(item: number, count: number = 1);
  Disable();
  DoReloot();
  Enable();
  GetAngle(): number[3];
  GetAnimationVariableBool(): boolean;
  GetBaseId(): number;
  GetCellOrWorld(): number;
  GetInventory(): Inventory;
  GetPos(): number[3];
  IsActivationBlocked(): boolean;
  IsDisabled(): boolean;
  IsHarvested(): boolean;
  IsOpen(): boolean;
  RemoveAllItems(container?: ObjectReference);
  RemoveItem(item: number, count: number = 1);
  SetActivationBlocked(isBlocked: boolean);
  SetAngle(angle: number[3]);
  SetAnimationVariableBool(animation: string, boolValue: boolean);
  SetCellOrWorld(cellOrWorldId: number);
  SetHarvested(IsHarvested: boolean);
  SetPos(angle: number[3]);
  MoveTo(akTarget: ObjectReference): void;
  PlaceAtMe(akFormToPlace: Form): number;
  GetDistance(obj: ObjectReference): number[3];
  GetLinkedDoorId(): number | undefined;
}
