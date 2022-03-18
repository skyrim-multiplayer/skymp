import { Flora, Form, FormType, MotionType, ObjectReference } from "skyrimPlatform";
import { NiPoint3 } from "./movement";
import { UtilsFormType } from "./utilsFormType";

export class UtilsObjectReference {
  static getWorldOrCell(self: ObjectReference): number {
    let world = self.getWorldSpace();
    if (world) return world.getFormID();

    let cell = self.getParentCell();
    if (cell) return cell.getFormID();

    return 0;
  }

  static getPos(self: ObjectReference): NiPoint3 {
    return [self.getPositionX(), self.getPositionY(), self.getPositionZ()];
  };

  static getDistance(a: NiPoint3, b: NiPoint3) {
    const deltaX = a[0] - b[0];
    const deltaY = a[1] - b[1];
    const deltaZ = a[2] - b[2];
    return Math.sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
  };

  static dealWithRef(self: ObjectReference, base: Form): void {
    const t = base.getType();
    const isItem = UtilsFormType.isItem(t);

    if (t === FormType.Container || isItem || t === FormType.Flora || t === FormType.Tree || t === FormType.NPC || t === FormType.Door) {
      self.blockActivation(true);
    } else {
      self.blockActivation(false);
    }

    if (self.isLocked()) {
      self.lock(false, false);
    }

    if (isItem) {
      self.setMotionType(MotionType.Keyframed, false);
    }

    // https://github.com/skyrim-multiplayer/issue-tracker/issues/36
    if (t === FormType.Flora && Flora.from(base)?.getIngredient()) {
      self.setMotionType(MotionType.Keyframed, false);
    }
  }
}
