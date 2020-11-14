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
  Form,
} from "skyrimPlatform";
import * as sp from "skyrimPlatform";

import { applyMovement, NiPoint3 } from "./components/movement";
import { applyAnimation } from "./components/animation";
import { Look, applyLook, applyTints } from "./components/look";
import { applyEquipment, isBadMenuShown } from "./components/equipment";
import { modWcProtection } from "./worldCleaner";
import { applyInventory } from "./components/inventory";

let gCrosshairRefId = 0;
let gPcInJumpState = false;
let gPcWorldOrCellId = 0;

export interface View<T> {
  update(model: T): void;
  destroy(): void;
}

const getFormEx = Game.getFormEx;

function isItem(t: number) {
  const isAmmo = t === 42;
  const isArmor = t === 26;
  const isBook = t === 27;
  const isIngredient = t === 30;
  const isLight = t === 31;
  const isPotion = t === 46;
  const isScroll = t === 23;
  const isSoulGem = t === 52;
  const isWeapon = t === 41;
  const isMisc = t === 32;

  const isItem =
    isAmmo ||
    isArmor ||
    isBook ||
    isIngredient ||
    isLight ||
    isPotion ||
    isScroll ||
    isSoulGem ||
    isWeapon ||
    isMisc;
  return isItem;
}

function dealWithRef(ref: ObjectReference, base: Form): void {
  const t = base.getType();
  const isContainer = t === 28;

  const isFlora = t === 39;
  const isTree = t === 38;

  const isIngredientSource = isFlora || isTree;

  const isMovableStatic = t === 36;
  const isNpc = t === 43;
  const isDoor = t === 29;

  if (isContainer || isItem(t) || isIngredientSource || isNpc || isDoor) {
    ref.blockActivation(true);
  } else {
    ref.blockActivation(false);
  }

  if (ref.isLocked()) {
    ref.lock(false, false);
  }

  if (isItem(t)) ref.setMotionType(MotionType.Keyframed, false);
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
  return { lastNumChanges: 0, isBadMenuShown: false, lastEqMoment: 0 };
};

interface LookState {
  lastNumChanges: number;
  look: Look;
}

const getDefaultLookState = (): LookState => {
  return { lastNumChanges: 0, look: null };
};

export class FormView implements View<FormModel> {
  constructor(private remoteRefrId?: number) {}

  update(model: FormModel): void {
    // Other players mutate into PC clones when moving to another location
    if (model.movement) {
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
    }

    // Players with different worldOrCell should be invisible
    if (model.movement) {
      const worldOrCell =
        Game.getPlayer().getWorldSpace() || Game.getPlayer().getParentCell();
      if (
        worldOrCell &&
        model.movement.worldOrCell !== worldOrCell.getFormID()
      ) {
        this.destroy();
        this.refrId = 0;
        return;
      }
    }

    // Apply look before base form selection to prevent double-spawn
    if (model.look) {
      if (
        !this.lookState.look ||
        model.numLookChanges !== this.lookState.lastNumChanges
      ) {
        this.lookState.look = model.look;
        this.lookState.lastNumChanges = model.numLookChanges;
        this.lookBasedBaseId = 0;
      }
    }

    const refId =
      model.refrId && model.refrId < 0xff000000 ? model.refrId : undefined;
    if (refId) {
      if (this.refrId !== refId) {
        this.destroy();
        this.refrId = model.refrId;
        this.ready = true;
        const refr = ObjectReference.from(Game.getFormEx(this.refrId));
        if (refr) {
          const base = refr.getBaseObject();
          if (base) dealWithRef(refr, base);
        }
      }
    } else {
      const base =
        getFormEx(+model.baseId) || getFormEx(this.getLookBasedBase());
      if (!base) return;

      let refr = ObjectReference.from(Game.getFormEx(this.refrId));
      const respawnRequired =
        !refr ||
        !refr.getBaseObject() ||
        refr.getBaseObject().getFormID() !== base.getFormID();

      if (respawnRequired) {
        this.destroy();
        refr = Game.getPlayer().placeAtMe(base, 1, true, true);
        const kTypeNpc = 43;
        if (base.getType() !== kTypeNpc) {
          refr.setAngle(
            model.movement.rot[0],
            model.movement.rot[1],
            model.movement.rot[2]
          );
        }
        modWcProtection(refr.getFormID(), 1);

        // TODO: reset all states?
        this.eqState = getDefaultEquipState();

        this.ready = false;
        new SpawnProcess(
          this.lookState.look,
          model.movement
            ? model.movement.pos
            : [
                Game.getPlayer().getPositionX(),
                Game.getPlayer().getPositionY(),
                Game.getPlayer().getPositionZ(),
              ],
          refr.getFormID(),
          () => {
            this.ready = true;
            this.spawnMoment = Date.now();
          }
        );
        if (model.look && model.look.name)
          refr.setDisplayName("" + model.look.name, true);
      }
      this.refrId = refr.getFormID();
    }

    if (!this.ready) return;

    const refr = ObjectReference.from(Game.getFormEx(this.refrId));
    if (refr) {
      this.applyAll(refr, model);
    }
  }

  destroy(): void {
    this.isOnScreen = false;
    this.spawnMoment = 0;
    const refr = ObjectReference.from(Game.getFormEx(this.refrId));
    if (this.refrId >= 0xff000000) {
      if (refr) refr.delete();
      modWcProtection(this.refrId, -1);
    }
  }

  private applyHarvested(refr: ObjectReference, isHarvested: boolean) {
    const base = refr.getBaseObject();
    if (base) {
      const t = base.getType();
      if (t >= 38 && t <= 39) {
        const wasHarvested = refr.isHarvested();
        if (isHarvested != wasHarvested) {
          let ac: Actor;
          if (isHarvested)
            for (let i = 0; i < 20; ++i) {
              ac = Game.findRandomActor(
                refr.getPositionX(),
                refr.getPositionY(),
                refr.getPositionZ(),
                10000
              );
              if (ac && ac.getFormID() !== 0x14) {
                break;
              }
            }
          if (isHarvested && ac && ac.getFormID() !== 0x14) {
            refr.activate(ac, true);
          } else {
            refr.setHarvested(isHarvested);
            const id = refr.getFormID();
            refr.disable(false).then(() => {
              const restoredRefr = ObjectReference.from(Game.getFormEx(id));
              if (restoredRefr) restoredRefr.enable(false);
            });
          }
        }
      } else {
        const wasHarvested = refr.isDisabled();
        if (isHarvested != wasHarvested) {
          if (isHarvested) {
            const id = refr.getFormID();
            refr.disable(false).then(() => {
              const restoredRefr = ObjectReference.from(Game.getFormEx(id));
              if (restoredRefr && !restoredRefr.isDisabled()) {
                restoredRefr.delete();
                // Deletion takes time, so in practice this would be called a lot of times
              }
            });
          } else refr.enable(true);
        }
      }
    }
  }

  private lastHarvestedApply = 0;
  private lastOpenApply = 0;

  private applyAll(refr: ObjectReference, model: FormModel) {
    let forcedWeapDrawn: boolean | null = null;

    if (gCrosshairRefId === this.refrId) {
      this.lastHarvestedApply = 0;
      this.lastOpenApply = 0;
    }
    const now = Date.now();
    if (now - this.lastHarvestedApply > 666) {
      this.lastHarvestedApply = now;
      this.applyHarvested(refr, !!model.isHarvested);
    }
    if (now - this.lastOpenApply > 133) {
      this.lastOpenApply = now;
      refr.setOpen(!!model.isOpen);
    }

    if (
      model.inventory &&
      gCrosshairRefId == this.refrId &&
      !isBadMenuShown()
    ) {
      // Do not let actors breaking their equipment via inventory apply
      // However, actually, actors do not have inventory in their models
      // Except your clone.
      if (!Actor.from(refr)) {
        applyInventory(refr, model.inventory, false, true);
      }
    }

    if (model.animation) {
      if (model.animation.animEventName === "SkympFakeUnequip") {
        forcedWeapDrawn = false;
      } else if (model.animation.animEventName === "SkympFakeEquip") {
        forcedWeapDrawn = true;
      }
    }

    if (model.movement) {
      const playerAllyFaction = sp.Faction.from(Game.getFormEx(0x0005a1a4));
      const ac = Actor.from(refr);
      if (ac) {
        //printConsole(model.isHostedByMe);
        if (model.isHostedByMe !== this.wasHostedByMe) {
          this.wasHostedByMe = model.isHostedByMe;
          if (model.isHostedByMe) {
            ac.removeFromFaction(playerAllyFaction);
          } else {
            ac.setFactionRank(playerAllyFaction, 1);
            ac.stopCombat();
          }
        }
      }

      if (
        +model.numMovementChanges !== this.movState.lastNumChanges ||
        Date.now() - this.movState.lastApply > 2000
      ) {
        this.movState.lastApply = Date.now();
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

    if (model.look) {
      const actor = Actor.from(refr);
      if (actor && !gPcInJumpState) {
        if (gPcWorldOrCellId) {
          if (
            this.lastPcWorldOrCell &&
            gPcWorldOrCellId !== this.lastPcWorldOrCell
          ) {
            // Redraw tints if PC world/cell changed
            this.isOnScreen = false;
          }
          this.lastPcWorldOrCell = gPcWorldOrCellId;
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
    }

    if (model.equipment) {
      const isShown = isBadMenuShown();
      if (this.eqState.isBadMenuShown !== isShown) {
        this.eqState.isBadMenuShown = isShown;
        if (!isShown) this.eqState.lastNumChanges = -1;
      }
      if (this.eqState.lastNumChanges !== model.equipment.numChanges) {
        const ac = Actor.from(refr);
        // If we do not block inventory here, we will be able to reproduce the bug:
        // 1. Place ~90 bots and force them to reequip iron swords to the left hand (rate should be ~50ms)
        // 2. Open your inventory and reequip different items fast
        // 3. After 1-2 minutes close your inventory and see that HUD disappeared
        if (
          ac &&
          !isBadMenuShown() &&
          Date.now() - this.eqState.lastEqMoment > 500 &&
          Date.now() - this.spawnMoment > -1 &&
          this.spawnMoment > 0
        ) {
          //if (this.spawnMoment > 0 && Date.now() - this.spawnMoment > 5000) {
          if (applyEquipment(ac, model.equipment)) {
            this.eqState.lastNumChanges = model.equipment.numChanges;
          }
          this.eqState.lastEqMoment = Date.now();
          //}
          //const res: boolean = applyEquipment(ac, model.equipment);
          //if (res) this.eqState.lastNumChanges = model.equipment.numChanges;
        }
      }
    }
  }

  private getLookBasedBase(): number {
    const base = ActorBase.from(Game.getFormEx(this.lookBasedBaseId));
    if (!base && this.lookState.look) {
      this.lookBasedBaseId = applyLook(this.lookState.look).getFormID();
    }
    return this.lookBasedBaseId;
  }

  getLocalRefrId(): number {
    return this.refrId;
  }

  getRemoteRefrId(): number {
    return this.remoteRefrId;
  }

  private refrId = 0;
  private ready = false;
  private animState = { lastNumChanges: 0 };
  private movState = { lastNumChanges: 0, lastApply: 0 };
  private lookState = getDefaultLookState();
  private eqState = getDefaultEquipState();
  private lookBasedBaseId = 0;
  private isOnScreen = false;
  private lastPcWorldOrCell = 0;
  private lastWorldOrCell = 0;
  private spawnMoment = 0;
  private wasHostedByMe: boolean | undefined = undefined;
}

class FormViewArray {
  updateForm(form: FormModel, i: number) {
    const view = this.formViews[i];
    if (!view) {
      this.formViews[i] = new FormView(form.refrId);
    } else {
      view.update(form);
    }
  }

  destroyForm(i: number) {
    if (!this.formViews[i]) return;
    this.formViews[i].destroy();
    this.formViews[i] = undefined;
  }

  resize(newSize: number) {
    if (this.formViews.length > newSize) {
      this.formViews.slice(newSize).forEach((v) => v && v.destroy());
    }
    this.formViews.length = newSize;
  }

  updateAll(model: WorldModel, showMe: boolean, isCloneView: boolean) {
    const toDestroy = new Array<number>();
    const forms = model.forms;
    const n = forms.length;
    for (let i = 0; i < n; ++i) {
      if (!forms[i] || (model.playerCharacterFormIdx === i && !showMe)) {
        this.destroyForm(i);
        continue;
      }
      const form = forms[i];

      let realPos: NiPoint3;
      const offset =
        form.movement && (model.playerCharacterFormIdx === i || isCloneView);
      if (offset) {
        realPos = form.movement.pos;
        form.movement.pos = [realPos[0] + 128, realPos[1] + 128, realPos[2]];
      }
      try {
        if (isCloneView) {
          const isHostedByMeBackup = form.isHostedByMe;
          form.isHostedByMe = false;
          this.updateForm(form, i);
          form.isHostedByMe = isHostedByMeBackup;
        } else this.updateForm(form, i);
      } catch (err) {
        if (err.message.includes("needs to be respawned")) {
          toDestroy.push(i);
          printConsole("destroying");
        } else {
          throw err;
        }
      }
      if (offset) {
        form.movement.pos = realPos;
      }
    }

    for (const i of toDestroy) this.destroyForm(i);
  }

  getRemoteRefrId(clientsideRefrId: number): number {
    if (clientsideRefrId < 0xff000000)
      throw new Error("This function is only for 0xff forms");
    const formView = this.formViews.find((formView: FormView) => {
      return formView && formView.getLocalRefrId() === clientsideRefrId;
    });
    return formView ? formView.getRemoteRefrId() : 0;
  }

  private formViews = new Array<FormView>();
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
          this.formViews.resize(0);
          this.cloneFormViews.resize(0);
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

  getRemoteRefrId(clientsideRefrId: number): number {
    return this.formViews.getRemoteRefrId(clientsideRefrId);
  }

  update(model: WorldModel): void {
    if (!this.allowUpdate) return;

    // Skip 50% of updates
    this.counter = !this.counter;
    if (this.counter) return;

    this.formViews.resize(model.forms.length);

    const showMe = settings["skymp5-client"]["show-me"];
    const showClones = settings["skymp5-client"]["show-clones"];

    const crosshair = Game.getCurrentCrosshairRef();
    gCrosshairRefId = crosshair ? crosshair.getFormID() : 0;

    gPcInJumpState = Game.getPlayer().getAnimationVariableBool("bInJumpState");

    const pcWorldOrCell =
      Game.getPlayer().getWorldSpace() || Game.getPlayer().getParentCell();
    gPcWorldOrCellId = pcWorldOrCell ? pcWorldOrCell.getFormID() : 0;

    this.formViews.updateAll(model, showMe, false);

    if (showClones) {
      this.cloneFormViews.updateAll(model, false, true);
    } else {
      this.cloneFormViews.resize(0);
    }
  }

  destroy(): void {
    this.formViews.resize(0);
  }

  private formViews = new FormViewArray();
  private cloneFormViews = new FormViewArray();
  private allowUpdate = false;
  private pcWorldOrCell = 0;
  private counter = false;
}
