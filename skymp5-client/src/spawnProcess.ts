import { ObjectReference, Game, Actor, MotionType } from "skyrimPlatform";
import { Appearance, applyTints } from "./appearance";
import { NiPoint3 } from "./movement";

export class SpawnProcess {
  constructor(
    appearance: Appearance,
    pos: NiPoint3,
    refrId: number,
    private callback: () => void
  ) {
    const refr = ObjectReference.from(Game.getFormEx(refrId));
    if (!refr || refr.getFormID() !== refrId) return;

    refr.setPosition(...pos).then(() => this.enable(appearance, refrId));
  }

  private enable(appearance: Appearance, refrId: number) {
    const refr = ObjectReference.from(Game.getFormEx(refrId));
    if (!refr || refr.getFormID() !== refrId) return;

    const ac = Actor.from(refr);
    if (appearance && ac) applyTints(ac, appearance);
    refr.enable(false).then(() => this.resurrect(appearance, refrId));
  }

  private resurrect(appearance: Appearance, refrId: number) {
    const refr = ObjectReference.from(Game.getFormEx(refrId));
    if (!refr || refr.getFormID() !== refrId) return;

    const ac = Actor.from(refr);
    if (ac) {
      return ac.resurrect().then(() => {
        this.callback();
      });
    }
    return refr.setMotionType(MotionType.Keyframed, true).then(this.callback);
  }
}