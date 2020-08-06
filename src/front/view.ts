import { FormModel, WorldModel } from "./model";
import {
  ObjectReference,
  Game,
  Actor,
  MotionType,
  settings,
  printConsole,
  ActorBase,
  once,
  on,
  Utility,
  worldPointToScreenPoint,
} from "skyrimPlatform";
import * as sp from "skyrimPlatform";

import { applyMovement, NiPoint3 } from "./components/movement";
import { applyAnimation } from "./components/animation";
import {
  Look,
  applyLook,
  applyTints,
  silentVoiceTypeId,
} from "./components/look";
import { applyEquipment, isBadMenuShown } from "./components/equipment";

export interface View<T> {
  update(model: T): void;
  destroy(): void;
}

// https://stackoverflow.com/questions/201183/how-to-determine-equality-for-two-javascript-objects
function objectEquals(x: any, y: any): boolean {
  "use strict";

  if (x === null || x === undefined || y === null || y === undefined) {
    return x === y;
  }
  // after this just checking type of one would be enough
  if (x.constructor !== y.constructor) {
    return false;
  }
  // if they are functions, they should exactly refer to same one (because of closures)
  if (x instanceof Function) {
    return x === y;
  }
  // if they are regexps, they should exactly refer to same one (it is hard to better equality check on current ES)
  if (x instanceof RegExp) {
    return x === y;
  }
  if (x === y || x.valueOf() === y.valueOf()) {
    return true;
  }
  if (Array.isArray(x) && x.length !== y.length) {
    return false;
  }

  // if they are dates, they must had equal valueOf
  if (x instanceof Date) {
    return false;
  }

  // if they are strictly equal, they both need to be object at least
  if (!(x instanceof Object)) {
    return false;
  }
  if (!(y instanceof Object)) {
    return false;
  }

  // recursive object equality check
  const p = Object.keys(x);
  return (
    Object.keys(y).every(function (i) {
      return p.indexOf(i) !== -1;
    }) &&
    p.every(function (i) {
      return objectEquals(x[i], y[i]);
    })
  );
}

class SpawnProcess {
  constructor(
    look: Look,
    pos: NiPoint3,
    refrId: number,
    private callback: () => void
  ) {
    const refr = ObjectReference.from(Game.getFormEx(refrId));
    if (!refr || refr.getFormID() !== refrId) return;

    refr.setPosition(...pos).then(() => this.enable(look, refrId));
  }

  private enable(look: Look, refrId: number) {
    const refr = ObjectReference.from(Game.getFormEx(refrId));
    if (!refr || refr.getFormID() !== refrId) return;

    const ac = Actor.from(refr);
    if (look && ac) applyTints(ac, look);
    refr.enable(false).then(() => this.resurrect(look, refrId));
  }

  private resurrect(look: Look, refrId: number) {
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

const getDefaultEquipState = () => {
  return { lastNumChanges: 0, isBadMenuShown: false };
};

export class FormView implements View<FormModel> {
  update(model: FormModel): void {
    if (!model.movement) {
      throw new Error("FormModel without Movement component, is it a mistake?");
    }

    // Other players mutate into PC clones when moving to another location
    if (!this.lastWorldOrCell)
      this.lastWorldOrCell = model.movement.worldOrCell;
    if (this.lastWorldOrCell !== model.movement.worldOrCell) {
      printConsole(
        `[1] worldOrCell changed, destroying FormView ${this.lastWorldOrCell.toString(
          16
        )} => ${model.movement.worldOrCell.toString(16)}`
      );
      this.lastWorldOrCell = model.movement.worldOrCell;
      this.destroy();
      this.refrId = 0;
      this.lookBasedBaseId = 0;
      return;
    }

    // Players with different worldOrCell should be invisible
    const worldOrCell =
      Game.getPlayer().getWorldSpace() || Game.getPlayer().getParentCell();
    if (worldOrCell && model.movement.worldOrCell !== worldOrCell.getFormID()) {
      this.destroy();
      this.refrId = 0;
      return;
    }

    // Apply look before base form selection to prevent double-spawn
    if (model.look) {
      if (!this.look || !objectEquals(model.look, this.look)) {
        this.look = model.look;
        this.lookBasedBaseId = 0;
      }
    }

    const AADeleteWhenDoneTestJeremyRegular = 0x0010d13e;
    const base =
      Game.getFormEx(+model.baseId) ||
      Game.getFormEx(this.getLookBasedBase()) ||
      Game.getFormEx(AADeleteWhenDoneTestJeremyRegular);

    let refr = ObjectReference.from(Game.getFormEx(this.refrId));
    const respawnRequired =
      !refr ||
      !refr.getBaseObject() ||
      refr.getBaseObject().getFormID() !== base.getFormID();

    if (respawnRequired) {
      this.destroy();
      refr = Game.getPlayer().placeAtMe(base, 1, true, true);

      // TODO: reset all states?
      this.eqState = getDefaultEquipState();

      this.ready = false;
      new SpawnProcess(
        this.look,
        model.movement.pos,
        refr.getFormID(),
        () => (this.ready = true)
      );
      if (model.look && model.look.name)
        refr.setDisplayName("" + model.look.name, true);
    }
    this.refrId = refr.getFormID();

    if (!this.ready) return;
    this.applyAll(refr, model);
  }

  destroy(): void {
    this.isOnScreen = false;
    const refr = ObjectReference.from(Game.getFormEx(this.refrId));
    if (refr) refr.delete();
  }

  private applyAll(refr: ObjectReference, model: FormModel) {
    let forcedWeapDrawn: boolean | null = null;

    if (model.animation) {
      if (model.animation.animEventName === "SkympFakeUnequip") {
        forcedWeapDrawn = false;
      } else if (model.animation.animEventName === "SkympFakeEquip") {
        forcedWeapDrawn = true;
      }
    }

    if (model.movement) {
      if (+model.numMovementChanges !== this.movState.lastNumChanges) {
        const backup = model.movement.isWeapDrawn;
        if (forcedWeapDrawn === true || forcedWeapDrawn === false) {
          model.movement.isWeapDrawn = forcedWeapDrawn;
        }
        applyMovement(refr, model.movement);
        model.movement.isWeapDrawn = backup;

        this.movState.lastNumChanges = +model.numMovementChanges;
      }
    }
    if (model.animation) applyAnimation(refr, model.animation, this.animState);

    const actor = Actor.from(refr);
    if (actor && !Game.getPlayer().getAnimationVariableBool("bInJumpState")) {
      const pcWorldOrCell =
        Game.getPlayer().getWorldSpace() || Game.getPlayer().getParentCell();
      if (pcWorldOrCell) {
        const id = pcWorldOrCell.getFormID();
        if (this.lastPcWorldOrCell && id !== this.lastPcWorldOrCell) {
          // Redraw tints if PC world/cell changed
          this.isOnScreen = false;
          /*this.destroy();
          this.refrId = 0;
          this.lastPcWorldOrCell = id;
          return;*/
        }
        this.lastPcWorldOrCell = id;
      }

      const ni = (sp as any)["NetImmerse"];
      const headPos = [
        ni.GetNodeWorldPositionX(actor, "NPC Head [Head]", false),
        ni.GetNodeWorldPositionY(actor, "NPC Head [Head]", false),
        ni.GetNodeWorldPositionZ(actor, "NPC Head [Head]", false),
      ];
      const [screenPoint] = worldPointToScreenPoint(headPos);
      const isOnScreen =
        screenPoint[0] > 0 &&
        screenPoint[1] > 0 &&
        screenPoint[2] > 0 &&
        screenPoint[0] < 1 &&
        screenPoint[1] < 1 &&
        screenPoint[2] < 1;
      if (isOnScreen != this.isOnScreen) {
        this.isOnScreen = isOnScreen;
        if (isOnScreen) {
          actor.queueNiNodeUpdate();
          Game.getPlayer().queueNiNodeUpdate();
        }
      }
    }

    if (model.equipment) {
      const isShown = isBadMenuShown();
      if (this.eqState.isBadMenuShown !== isShown) {
        this.eqState.isBadMenuShown = isShown;
        if (!isShown) this.eqState.lastNumChanges = -1;
      }
      if (this.eqState.lastNumChanges !== model.equipment.numChanges) {
        const ac = Actor.from(refr);
        if (ac) {
          const res: boolean = applyEquipment(ac, model.equipment);
          if (res) this.eqState.lastNumChanges = model.equipment.numChanges;
        }
      }
    }
  }

  private getLookBasedBase(): number {
    const base = ActorBase.from(Game.getFormEx(this.lookBasedBaseId));
    if (!base && this.look) {
      this.lookBasedBaseId = applyLook(this.look).getFormID();
    }
    return this.lookBasedBaseId;
  }

  private refrId = 0;
  private ready = false;
  private animState = { lastNumChanges: 0 };
  private movState = { lastNumChanges: 0 };
  private eqState = getDefaultEquipState();
  private lookBasedBaseId = 0;
  private look?: Look;
  private isOnScreen = false;
  private lastPcWorldOrCell = 0;
  private lastWorldOrCell = 0;
}

export class WorldView implements View<WorldModel> {
  constructor() {
    // Work around showRaceMenu issue
    // Default nord in Race Menu will have very ugly face
    // If other players are spawning when we show this menu
    on("update", () => {
      const pc = Game.getPlayer();
      const pcWorldOrCell = (
        pc.getWorldSpace() || pc.getParentCell()
      ).getFormID();
      if (this.pcWorldOrCell !== pcWorldOrCell) {
        if (this.pcWorldOrCell) {
          printConsole("Reset all form views");
          for (let i = 0; i < this.formViews.length; ++i) {
            this.destroyForm(i);
          }
        }
        this.pcWorldOrCell = pcWorldOrCell;
      }
    });
    once("update", () => {
      // Wait 1s game time (time spent in Race Menu isn't counted)
      Utility.wait(1).then(() => {
        this.allowUpdate = true;
        printConsole("Update is now allowed");
      });
    });
  }

  update(model: WorldModel): void {
    if (!this.allowUpdate) return;

    this.resize(model.forms.length);

    const showMe = settings["skymp5-client"]["show-me"];

    const toDestroy = new Array<number>();

    model.forms.forEach((form, i) => {
      if (!form || (model.playerCharacterFormIdx === i && !showMe)) {
        return this.destroyForm(i);
      }

      let realPos: NiPoint3;
      if (model.playerCharacterFormIdx === i && form.movement) {
        realPos = form.movement.pos;
        form.movement.pos = [realPos[0] + 128, realPos[1] + 128, realPos[2]];
      }
      try {
        this.updateForm(form, i);
      } catch (err) {
        if (err.message.includes("needs to be respawned")) {
          toDestroy.push(i);
          printConsole("destroying");
        } else {
          throw err;
        }
      }
      if (model.playerCharacterFormIdx === i && form.movement) {
        form.movement.pos = realPos;
      }
    });

    toDestroy.forEach((i) => this.destroyForm(i));
  }

  private updateForm(form: FormModel, i: number) {
    if (!this.formViews[i]) this.formViews[i] = new FormView();
    this.formViews[i].update(form);
  }

  private destroyForm(i: number) {
    if (!this.formViews[i]) return;
    this.formViews[i].destroy();
    this.formViews[i] = undefined;
  }

  private resize(newSize: number) {
    if (this.formViews.length > newSize) {
      this.formViews.slice(newSize).forEach((v) => v && v.destroy());
    }
    this.formViews.length = newSize;
  }

  destroy(): void {
    this.resize(0);
  }

  private formViews = new Array<FormView>();
  private allowUpdate = false;
  private pcWorldOrCell = 0;
}
