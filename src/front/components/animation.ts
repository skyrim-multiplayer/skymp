/* eslint-disable @typescript-eslint/no-empty-function */
import {
  ObjectReference,
  Debug,
  hooks,
  Actor,
  printConsole,
} from "skyrimPlatform";
import { Movement } from "./movement";
import { applyWeapDrawn } from "./movementApply";

export interface Animation {
  animEventName: string;
  numChanges: number;
}

export interface AnimationApplyState {
  lastNumChanges: number;
}

const isIdle = (animEventName: string) => {
  return (
    animEventName === "MotionDrivenIdle" ||
    (animEventName.startsWith("Idle") &&
      animEventName !== "IdleStop" &&
      animEventName !== "IdleForceDefaultState")
  );
};

const allowedIdles = new Array<[number, string]>();

export const applyAnimation = (
  refr: ObjectReference,
  anim: Animation,
  state: AnimationApplyState
): void => {
  if (state.lastNumChanges === anim.numChanges) return;
  state.lastNumChanges = anim.numChanges;

  if (isIdle(anim.animEventName)) {
    allowedIdles.push([refr.getFormID(), anim.animEventName]);
  }

  if (anim.animEventName === "SkympFakeEquip") {
    const ac = Actor.from(refr);
    if (ac) applyWeapDrawn(ac, true);
  } else if (anim.animEventName === "SkympFakeUnequip") {
    const ac = Actor.from(refr);
    if (ac) applyWeapDrawn(ac, false);
  } else {
    Debug.sendAnimationEvent(refr, anim.animEventName);
  }
};

export class AnimationSource {
  constructor(refr: ObjectReference) {
    this.refrId = refr.getFormID();
    hooks.sendAnimationEvent.add({
      enter: () => {},
      leave: (ctx) => {
        if (ctx.selfId !== this.refrId) return;
        if (!ctx.animationSucceeded) return;
        this.onSendAnimationEvent(ctx.animEventName);
      },
    });
  }

  filterMovement(mov: Movement): Movement {
    if (this.weapDrawnBlocker >= Date.now()) mov.isWeapDrawn = true;
    if (this.weapNonDrawnBlocker >= Date.now()) mov.isWeapDrawn = false;

    if (this.sneakBlocker === mov.isSneaking) this.sneakBlocker = null;
    else if (this.sneakBlocker === true) mov.isSneaking = true;
    else if (this.sneakBlocker === false) mov.isSneaking = false;

    return mov;
  }

  getAnimation(): Animation {
    const { numChanges, animEventName } = this;
    return { numChanges, animEventName };
  }

  private onSendAnimationEvent(animEventName: string) {
    if (ignoredAnims.has(animEventName)) return;

    const lower = animEventName.toLowerCase();
    if (lower.includes("spell")) {
      printConsole("spell animation has been blocked");
      return;
    }

    const isTorchEvent = lower.includes("torch");
    if (animEventName.toLowerCase().includes("unequip") && !isTorchEvent) {
      this.weapNonDrawnBlocker = Date.now() + 300;
      animEventName = "SkympFakeUnequip";
    } else if (animEventName.toLowerCase().includes("equip") && !isTorchEvent) {
      this.weapDrawnBlocker = Date.now() + 300;
      animEventName = "SkympFakeEquip";
    }

    if (animEventName === "SneakStart") {
      this.sneakBlocker = true;
      return;
    }
    if (animEventName === "SneakStop") {
      this.sneakBlocker = false;
      return;
    }

    if (animEventName === "Ragdoll") return;

    this.numChanges++;
    this.animEventName = animEventName;
  }

  private refrId = 0;
  private numChanges = 0;
  private animEventName = "";

  private weapNonDrawnBlocker = 0;
  private weapDrawnBlocker = 0;
  private sneakBlocker: boolean | null = null;
}

const ignoredAnims = new Set<string>([
  "moveStart",
  "moveStop",
  "turnStop",
  "CyclicCrossBlend",
  "CyclicFreeze",
  "TurnLeft",
  "TurnRight",
]);

export const setupHooks = (): void => {
  hooks.sendAnimationEvent.add({
    enter: (ctx) => {
      // ShowRaceMenu forces this anim
      if (ctx.animEventName === "OffsetBoundStandingPlayerInstant") {
        return (ctx.animEventName = "");
      }

      // Disable idle animations for 0xff actors
      if (ctx.selfId < 0xff000000) return;
      if (isIdle(ctx.animEventName)) {
        const i = allowedIdles.findIndex((pair) => {
          return pair[0] === ctx.selfId && pair[1] === ctx.animEventName;
        });
        i === -1 ? (ctx.animEventName = "") : allowedIdles.splice(i, 1);
      }
    },
    leave: () => {},
  });
};
