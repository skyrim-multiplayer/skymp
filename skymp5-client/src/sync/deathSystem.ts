import {
  hooks,
  Game,
  Actor,
  Debug,
  once
} from "skyrimPlatform";
import { setLocalDamageMult, defaultLocalDamageMult } from "../index";
import { AnimationEventName } from "./animation";
import { RespawnNeededError } from "../lib/errors";

/**
 * Null to allow all animations. Empty array to disallow all
 */
let gPlayerAllowAnimations: string[] | null = null;
const gPlayerId: number = 0x14;

// Turn off kill move animations
hooks.sendAnimationEvent.add(
  {
    enter(ctx) {
      ctx.animEventName = "";
    },
    leave() {},
  },
  0,
  0xffffffff,
  "KillMove*"
);

// Turn off stagger animations
hooks.sendAnimationEvent.add(
  {
    enter(ctx) {
      ctx.animEventName = "";
    },
    leave() {},
  },
  0xff000000,
  0xffffffff,
  "staggerStart"
);

hooks.sendAnimationEvent.add(
  {
    enter(ctx) {
      if (!gPlayerAllowAnimations) return;
      if (!gPlayerAllowAnimations.includes(ctx.animEventName)) {
        ctx.animEventName = "";
      }
    },
    leave() {},
  },
  gPlayerId,
  gPlayerId
);

const isPlayer = (actor: Actor): boolean => {
  return actor.getFormID() === gPlayerId;
};

export const makeActorImmortal = (act: Actor): void => {
  act.startDeferredKill();
};

export const makeActorMortal = (act: Actor): void => {
  act.endDeferredKill();
};

export const applyDeathState = (actor: Actor, isDead: boolean) => {
  if (actor.isDead() === isDead && isPlayer(actor) === false) return;
  if (isDead === true) {
    killActor(actor, null);
  } else {
    resurrectActor(actor);
  }
};

export const safeRemoveRagdollFromWorld = (
  actor: Actor,
  afterRemoveCallback: () => void
) => {
  setLocalDamageMult(0);
  actor.forceRemoveRagdollFromWorld().then(() => {
    once("update", () => {
      setLocalDamageMult(defaultLocalDamageMult);
      afterRemoveCallback();
    });
  });
};

const killActor = (act: Actor, killer: Actor | null = null): void => {
  if (isPlayer(act) === true) {
    gPlayerAllowAnimations = [];
    act.setDontMove(true);
    killWithPush(act);
  } else {
    makeActorMortal(act);
    act.kill(killer);
  }
};

const resurrectActor = (act: Actor): void => {
  if (isPlayer(act) === true) {
    gPlayerAllowAnimations = null;
    act.setDontMove(false);
    ressurectWithPushKill(act);
  } else {
    throw new RespawnNeededError("needs to be respawned");
  }
};

const killWithPush = (act: Actor): void => {
  gPlayerAllowAnimations?.push(AnimationEventName.Ragdoll);
  act.pushActorAway(act, 0);
};

const ressurectWithPushKill = (act: Actor): void => {
  const formId = act.getFormID();
  safeRemoveRagdollFromWorld(act, () => {
    const actor = Actor.from(Game.getFormEx(formId));
    if (!actor) return;
    Game.getPlayer()!.setAnimationVariableInt("iGetUpType", 1);
    Debug.sendAnimationEvent(actor, AnimationEventName.GetUpBegin);
  });
};
