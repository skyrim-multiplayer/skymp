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
  storage,
  NetImmerse,
} from "skyrimPlatform";
import * as sp from "skyrimPlatform";

import { applyMovement, NiPoint3 } from "./movement";
import { applyAnimation, setDefaultAnimsDisabled } from "./animation";
import { Appearance, applyAppearance, applyTints } from "./appearance";
import { applyEquipment, isBadMenuShown } from "./equipment";
import { modWcProtection } from "./worldCleaner";
import { applyInventory } from "./inventory";
import { tryHost } from "./hostAttempts";
import { getMovement } from "./movementGet";
import { Movement } from "./movement";
import * as deathSystem from "./deathSystem";
import { RespawnNeededError } from "./errors";
import { getScreenResolution } from "./skyrimSettings";

let gCrosshairRefId = 0;
let gPcInJumpState = false;
let gPcWorldOrCellId = 0;
let gUpdateNeighborFunctionsKeys = new Array<string>();
let gUpdateNeighborFunctions: Record<string, any> = {};

on("tick", () => {
  const keys = storage["updateNeighborFunctions_keys"] as Array<string>;
  if (keys && Array.isArray(keys)) {
    gUpdateNeighborFunctionsKeys = keys;
  } else {
    gUpdateNeighborFunctionsKeys = [];
  }
  gUpdateNeighborFunctions = storage["updateNeighborFunctions"] as Record<
    string,
    any
  >;
});

export interface View<T> {
  update(model: T): void;
  destroy(): void;
}

const getFormEx = Game.getFormEx;
const headPart = "NPC Head [Head]";
const maxNicknameDrawDistance = 1000;

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

const lastTryHost: Record<number, number> = {};

const tryHostIfNeed = (ac: Actor, remoteId: number) => {
  const last = lastTryHost[remoteId];
  if (!last || Date.now() - last >= 1000) {
    lastTryHost[remoteId] = Date.now();

    if (
      getMovement(ac).worldOrCell ===
      getMovement(Game.getPlayer() as Actor).worldOrCell
    ) {
      return tryHost(remoteId);
    }
  }
};

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

  if (isItem(t)) {
    ref.setMotionType(MotionType.Keyframed, false);
  }
  // https://github.com/skyrim-multiplayer/issue-tracker/issues/36
  if (isFlora) {
    const hasIngr = (sp.Flora.from(base) as sp.Flora).getIngredient() != null;
    if (hasIngr) ref.setMotionType(MotionType.Keyframed, false);
  }
}

class SpawnProcess {
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

const getDefaultEquipState = () => {
  return { lastNumChanges: 0, isBadMenuShown: false, lastEqMoment: 0 };
};

interface AppearanceState {
  lastNumChanges: number;
  appearance: Appearance | null;
}

const getDefaultAppearanceState = (): AppearanceState => {
  return { lastNumChanges: 0, appearance: null };
};

const undefinedRefr: ObjectReference = undefined as unknown as ObjectReference;
const unknownValue: unknown = undefined;
const undefinedFormModel: FormModel = undefined as unknown as FormModel;
const undefinedObject: Record<string, unknown> = undefined as unknown as Record<string, unknown>;
const undefinedView: FormViewArray = undefined as unknown as FormViewArray;
const ctx = {
  refr: undefinedRefr,
  value: unknownValue,
  _model: undefinedFormModel,
  sp: sp,
  state: undefinedObject,
  _view: undefinedView,
  i: -1,
  getFormIdInServerFormat: (clientsideFormId: number) => {
    return localIdToRemoteId(clientsideFormId);
  },
  getFormIdInClientFormat: (serversideFormId: number) => {
    return remoteIdToLocalId(serversideFormId);
  },
  get(propName: string) {
    return (this._model as Record<string, any>)[propName];
  },
  respawn() {
    this._view.destroyForm(this.i);
  },
};

export class FormView implements View<FormModel> {
  constructor(private remoteRefrId?: number) { }

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
        this.appearanceBasedBaseId = 0;
        return;
      }
    }

    // Players with different worldOrCell should be invisible
    if (model.movement) {
      const worldOrCell =
        (Game.getPlayer() as Actor).getWorldSpace() ||
        (Game.getPlayer() as Actor).getParentCell();
      if (
        worldOrCell &&
        model.movement.worldOrCell !== worldOrCell.getFormID()
      ) {
        this.destroy();
        this.refrId = 0;
        return;
      }
    }

    // Apply appearance before base form selection to prevent double-spawn
    if (model.appearance) {
      if (
        !this.appearanceState.appearance ||
        model.numAppearanceChanges !== this.appearanceState.lastNumChanges
      ) {
        this.appearanceState.appearance = model.appearance;
        this.appearanceState.lastNumChanges = model.numAppearanceChanges as number;
        this.appearanceBasedBaseId = 0;
      }
    }

    const refId =
      model.refrId && model.refrId < 0xff000000 ? model.refrId : undefined;
    if (refId) {
      if (this.refrId !== refId) {
        this.destroy();
        this.refrId = model.refrId as number;
        this.ready = true;
        const refr = ObjectReference.from(Game.getFormEx(this.refrId));
        if (refr) {
          const base = refr.getBaseObject();
          if (base) dealWithRef(refr, base);
        }
      }
    } else {
      const base =
        getFormEx(+(model.baseId as number)) ||
        getFormEx(this.getAppearanceBasedBase());
      if (!base) return;

      let refr = ObjectReference.from(Game.getFormEx(this.refrId));
      const respawnRequired =
        !refr ||
        !refr.getBaseObject() ||
        (refr.getBaseObject() as Form).getFormID() !== base.getFormID();

      if (respawnRequired) {
        this.destroy();
        refr = (Game.getPlayer() as Actor).placeAtMe(
          base,
          1,
          true,
          true
        ) as ObjectReference;
        this.state = {};
        delete this.wasHostedByOther;
        const kTypeNpc = 43;
        if (base.getType() !== kTypeNpc) {
          refr.setAngle(
            model.movement?.rot[0] || 0,
            model.movement?.rot[1] || 0,
            model.movement?.rot[2] || 0
          );
        }
        modWcProtection(refr.getFormID(), 1);

        // TODO: reset all states?
        this.eqState = getDefaultEquipState();

        this.ready = false;
        new SpawnProcess(
          this.appearanceState.appearance as Appearance,
          model.movement
            ? model.movement.pos
            : [
              (Game.getPlayer() as Actor).getPositionX(),
              (Game.getPlayer() as Actor).getPositionY(),
              (Game.getPlayer() as Actor).getPositionZ(),
            ],
          refr.getFormID(),
          () => {
            this.ready = true;
            this.spawnMoment = Date.now();
          }
        );
        if (model.appearance && model.appearance.name)
          refr.setDisplayName("", true);
        Actor.from(refr)?.setActorValue("attackDamageMult", 0);
      }
      this.refrId = (refr as ObjectReference).getFormID();
    }

    if (!this.ready) return;

    const refr = ObjectReference.from(Game.getFormEx(this.refrId));
    if (refr) {
      const actor = Actor.from(refr);
      if (actor && !this.localImmortal) {
        deathSystem.makeActorImmortal(actor);
        actor.setActorValue("health", 1000000);
        this.localImmortal = true;
      }
      this.applyAll(refr, model);
      for (const key of gUpdateNeighborFunctionsKeys) {
        const v = (model as Record<string, unknown>)[key];
        // From docs:
        // In `updateOwner`/`updateNeighbor` equals to a value of a currently processed property.
        // Can't be `undefined` here, since updates are not received for `undefined` property values.
        // In other contexts is always `undefined`.
        if (v !== undefined) {
          if (this.refrId >= 0xff000000) {
            /*printConsole(
              "upd",
              this.refrId.toString(16),
              `${key}=${JSON.stringify(v)}`
            );*/
          }
          ctx.refr = refr;
          ctx.value = v;
          ctx._model = model;
          ctx.state = this.state;
          const f = gUpdateNeighborFunctions[key];
          // Actually, 'f' should always be a valid function, but who knows
          try {
            if (f) f(ctx);
          } catch (e) {
            printConsole(`'updateNeighbor.${key}' - `, e);
          }
        }
      }
    }
  }

  destroy(): void {
    this.isOnScreen = false;
    this.spawnMoment = 0;
    const refr = ObjectReference.from(Game.getFormEx(this.refrId));
    if (this.refrId >= 0xff000000) {
      if (refr) refr.delete();
      modWcProtection(this.refrId, -1);
      const ac = Actor.from(refr);
      if (ac) {
        sp.TESModPlatform.setWeaponDrawnMode(ac, -1);
      }
    }
    this.localImmortal = false;
    if (this.textNameId) {
      sp.destroyText(this.textNameId);
    }
  }

  private applyHarvested(refr: ObjectReference, isHarvested: boolean) {
    const base = refr.getBaseObject();
    if (base) {
      const t = base.getType();
      if (t >= 38 && t <= 39) {
        const wasHarvested = refr.isHarvested();
        if (isHarvested != wasHarvested) {
          let ac: Actor = undefined as unknown as Actor;
          if (isHarvested)
            for (let i = 0; i < 20; ++i) {
              ac = Game.findRandomActor(
                refr.getPositionX(),
                refr.getPositionY(),
                refr.getPositionZ(),
                10000
              ) as Actor;
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
      const ac = Actor.from(refr);
      if (ac) {
        if (model.isHostedByOther !== this.wasHostedByOther) {
          this.wasHostedByOther = model.isHostedByOther;
          this.movState.lastApply = 0;
          if (model.isHostedByOther) {
            setDefaultAnimsDisabled(ac.getFormID(), true);
          } else {
            setDefaultAnimsDisabled(ac.getFormID(), false);
          }
        }
      }

      if (
        this.movState.lastApply &&
        Date.now() - this.movState.lastApply > 1500
      ) {
        if (Date.now() - this.movState.lastRehost > 1000) {
          this.movState.lastRehost = Date.now();
          const remoteId = this.remoteRefrId;
          if (ac && ac.is3DLoaded()) {
            tryHostIfNeed(ac, remoteId as number);
          }
        }
      }

      if (
        +(model.numMovementChanges as number) !==
        this.movState.lastNumChanges ||
        Date.now() - this.movState.lastApply > 2000
      ) {
        this.movState.lastApply = Date.now();
        if (model.isHostedByOther || !this.movState.everApplied) {
          const backup = model.movement.isWeapDrawn;
          if (forcedWeapDrawn === true || forcedWeapDrawn === false) {
            model.movement.isWeapDrawn = forcedWeapDrawn;
          }
          try {
            applyMovement(refr, model.movement);
          } catch (e) {
            if (e instanceof RespawnNeededError) {
              this.lastWorldOrCell = model.movement.worldOrCell;
              this.destroy();
              this.refrId = 0;
              this.appearanceBasedBaseId = 0;
              return;
            } else {
              throw e;
            }
          }
          model.movement.isWeapDrawn = backup;

          this.movState.lastNumChanges = +(model.numMovementChanges as number);
          this.movState.everApplied = true;
        } else {
          if (ac) ac.clearKeepOffsetFromActor();
          if (ac) sp.TESModPlatform.setWeaponDrawnMode(ac, -1);
          const remoteId = this.remoteRefrId;
          if (ac && remoteId && ac.is3DLoaded()) tryHostIfNeed(ac, remoteId);
        }
      }
    }
    if (model.animation) applyAnimation(refr, model.animation, this.animState);

    if (model.appearance) {
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

        const headPos = [
          NetImmerse.getNodeWorldPositionX(actor, "NPC Head [Head]", false),
          NetImmerse.getNodeWorldPositionY(actor, "NPC Head [Head]", false),
          NetImmerse.getNodeWorldPositionZ(actor, "NPC Head [Head]", false),
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
            (Game.getPlayer() as Actor).queueNiNodeUpdate();
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

    if (model.appearance?.name) {
      const playerActor = sp.Game.getPlayer()!;
      // const isVisibleByPlayer = playerActor.hasLOS(refr) && playerActor.getDistance(refr) <= 200;
      const isVisibleByPlayer = playerActor.getDistance(refr) <= maxNicknameDrawDistance;
      if (isVisibleByPlayer) {
        const headScreenPos = sp.worldPointToScreenPoint([
          sp.NetImmerse.getNodeWorldPositionX(refr, headPart, false),
          sp.NetImmerse.getNodeWorldPositionY(refr, headPart, false),
          sp.NetImmerse.getNodeWorldPositionZ(refr, headPart, false) + 22
        ])[0];
        const resolution = getScreenResolution();
        const textXPos = Math.round(headScreenPos[0] * resolution.width);
        const textYPos = Math.round((1 - headScreenPos[1]) * resolution.height);

        if (!this.textNameId) {
          this.textNameId = sp.createText(textXPos, textYPos, model.appearance.name, [255, 255, 255, 1]);
        } else {
          sp.setTextString(this.textNameId, model.appearance.name);
          sp.setTextPos(this.textNameId, textXPos, textYPos);
        }
      } else if (this.textNameId) {
        sp.destroyText(this.textNameId);
        this.textNameId = undefined;
      }
    }
  }

  private getAppearanceBasedBase(): number {
    const base = ActorBase.from(Game.getFormEx(this.appearanceBasedBaseId));
    if (!base && this.appearanceState.appearance) {
      this.appearanceBasedBaseId = applyAppearance(this.appearanceState.appearance).getFormID();
    }
    return this.appearanceBasedBaseId;
  }

  getLocalRefrId(): number {
    return this.refrId;
  }

  getRemoteRefrId(): number {
    return this.remoteRefrId as number;
  }

  private refrId = 0;
  private ready = false;
  private animState = { lastNumChanges: 0 };
  private movState = {
    lastNumChanges: 0,
    lastApply: 0,
    lastRehost: 0,
    everApplied: false,
  };
  private appearanceState = getDefaultAppearanceState();
  private eqState = getDefaultEquipState();
  private appearanceBasedBaseId = 0;
  private isOnScreen = false;
  private lastPcWorldOrCell = 0;
  private lastWorldOrCell = 0;
  private spawnMoment = 0;
  private wasHostedByOther: boolean | undefined = undefined;
  private state = {};
  private localImmortal = false;
  private textNameId: number | undefined = undefined;
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
    this.formViews[i] = undefined as unknown as FormView;
  }

  resize(newSize: number) {
    if (this.formViews.length > newSize) {
      this.formViews.slice(newSize).forEach((v) => v && v.destroy());
    }
    this.formViews.length = newSize;
  }

  updateAll(model: WorldModel, showMe: boolean, isCloneView: boolean) {
    ctx._view = this;
    const forms = model.forms;
    const n = forms.length;
    for (let i = 0; i < n; ++i) {
      if (!forms[i] || (model.playerCharacterFormIdx === i && !showMe)) {
        this.destroyForm(i);
        continue;
      }
      const form = forms[i];

      let realPos: NiPoint3 = undefined as unknown as NiPoint3;
      const offset =
        form.movement && (model.playerCharacterFormIdx === i || isCloneView);
      if (offset) {
        realPos = (form.movement as Movement).pos;
        (form.movement as Movement).pos = [
          realPos[0] + 128,
          realPos[1] + 128,
          realPos[2],
        ];
      }
      if (isCloneView) {
        // Prevent using the same refr by normal and clone views
        if (!form.refrId || form.refrId >= 0xff000000) {
          const backup = form.isHostedByOther;
          form.isHostedByOther = true;
          this.updateForm(form, i);
          form.isHostedByOther = backup;
        }
      } else {
        ctx.i = i;
        this.updateForm(form, i);
      }

      if (offset) {
        (form.movement as Movement).pos = realPos as NiPoint3;
      }
    }
  }

  getRemoteRefrId(clientsideRefrId: number): number {
    if (clientsideRefrId < 0xff000000)
      throw new Error("This function is only for 0xff forms");
    const formView = this.formViews.find((formView: FormView) => {
      return formView && formView.getLocalRefrId() === clientsideRefrId;
    });
    return formView ? formView.getRemoteRefrId() : 0;
  }

  getLocalRefrId(remoteRefrId: number): number {
    if (remoteRefrId < 0xff000000)
      throw new Error("This function is only for 0xff forms");
    const formView = this.formViews.find((formView: FormView) => {
      return formView && formView.getRemoteRefrId() === remoteRefrId;
    });
    return formView ? formView.getLocalRefrId() : 0;
  }

  private formViews = new Array<FormView>();
}

export class WorldView implements View<WorldModel> {
  constructor() {
    // Work around showRaceMenu issue
    // Default nord in Race Menu will have very ugly face
    // If other players are spawning when we show this menu
    on("update", () => {
      const pc = Game.getPlayer() as Actor;
      const pcWorldOrCell = (
        (pc.getWorldSpace() || pc.getParentCell()) as Form
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
    sp.destroyAllTexts();
  }

  getRemoteRefrId(clientsideRefrId: number): number {
    return this.formViews.getRemoteRefrId(clientsideRefrId);
  }

  getLocalRefrId(remoteRefrId: number): number {
    return this.formViews.getLocalRefrId(remoteRefrId);
  }

  update(model: WorldModel): void {
    if (!this.allowUpdate) return;

    this.formViews.resize(model.forms.length);

    const showMe = settings["skymp5-client"]["show-me"];
    const showClones = settings["skymp5-client"]["show-clones"];

    const crosshair = Game.getCurrentCrosshairRef();
    gCrosshairRefId = crosshair ? crosshair.getFormID() : 0;

    gPcInJumpState = (Game.getPlayer() as Actor).getAnimationVariableBool(
      "bInJumpState"
    );

    const pcWorldOrCell =
      (Game.getPlayer() as Actor).getWorldSpace() ||
      (Game.getPlayer() as Actor).getParentCell();
    gPcWorldOrCellId = pcWorldOrCell ? pcWorldOrCell.getFormID() : 0;

    this.formViews.updateAll(model, showMe as boolean, false);

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
}

export const getViewFromStorage = (): WorldView | undefined => {
  const res = storage.view as WorldView;
  if (typeof res === "object") return res;
  return undefined;
};

export const localIdToRemoteId = (localFormId: number): number => {
  if (localFormId >= 0xff000000) {
    const view = getViewFromStorage();
    if (!view) return 0;
    localFormId = view.getRemoteRefrId(localFormId);
    if (!localFormId) return 0;
    // serverside ids are 64bit
    if (localFormId >= 0x100000000) {
      localFormId -= 0x100000000;
    }
  }
  return localFormId;
};

export const remoteIdToLocalId = (remoteFormId: number): number => {
  if (remoteFormId >= 0xff000000) {
    const view = getViewFromStorage();
    if (!view) return 0;
    remoteFormId = view.getLocalRefrId(remoteFormId);
    if (!remoteFormId) return 0;
  }
  return remoteFormId;
};
