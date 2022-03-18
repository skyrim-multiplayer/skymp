import { ObjectReference } from "skyrimPlatform";
import { NiPoint3 } from "./movement";

export class ObjectReferenceEx {
  static getWorldOrCell(self: ObjectReference): number {
    let world = self.getWorldSpace();
    if (world) return world.getFormID();

    let cell = self.getParentCell();
    if (cell) return cell.getFormID();

    return 0;
  }

  static getPos = (self: ObjectReference): NiPoint3 => {
    return [self.getPositionX(), self.getPositionY(), self.getPositionZ()];
  };

  static getDistance = (a: NiPoint3, b: NiPoint3) => {
    const deltaX = a[0] - b[0];
    const deltaY = a[1] - b[1];
    const deltaZ = a[2] - b[2];
    return Math.sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
  };
}