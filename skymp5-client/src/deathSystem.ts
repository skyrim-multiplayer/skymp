import { hooks, Game, printConsole, Actor, Debug, once, ObjectReference, Utility } from "skyrimPlatform";
import { AnimationEventName } from "./animation";
import { RespawnNeededError } from "./errors";

/*
You can kill if you want
If you want it you will kill
Oh, come on, kill with push
For a property death state
...
*/
enum KillRessurectStrategies {
  Push = 1,
  Anim = 2,
}

/**
 * Null for allow all. Empty array for disallow all
 */
let gPlayerAllowAnimations: string[] | null = null;
let gPlayerId: number = 0x14;
let gKillRessurectStrategy: KillRessurectStrategies = KillRessurectStrategies.Push;

// Blocking kill move animations
hooks.sendAnimationEvent.add({
  enter(ctx) {
    ctx.animEventName = "";
  },
  leave() { }
}, 0, 0xffffffff, "KillMove*");

hooks.sendAnimationEvent.add({
  enter(ctx) {
    if (!gPlayerAllowAnimations) return;
    if (!gPlayerAllowAnimations.includes(ctx.animEventName)) {
      ctx.animEventName = "";
    }
  },
  leave() { }
}, gPlayerId, gPlayerId);

const isPlayer = (actor: Actor): boolean => {
  return actor.getFormID() === gPlayerId;
}

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

const killActor = (act: Actor, killer: Actor | null = null): void => {
  if (isPlayer(act) === true) {
    gPlayerAllowAnimations = [];
    act.setDontMove(true);

    switch (gKillRessurectStrategy) {
      case KillRessurectStrategies.Push:
        killWithPush(act);
        break;
      case KillRessurectStrategies.Anim:
        killWithAnim(act);
        break;
    }

  } else {
    makeActorMortal(act);
    act.kill(killer);
  }
}

const resurrectActor = (act: Actor): void => {
  if (isPlayer(act) === true) {
    gPlayerAllowAnimations = null;
    act.setDontMove(false);
    switch (gKillRessurectStrategy) {
      case KillRessurectStrategies.Push:
        ressurectWithPushKill(act);
        break;
      case KillRessurectStrategies.Anim:
        ressurectWithAnimKill(act);
        break;
    }
  } else {
    makeActorImmortal(act);
    throw new RespawnNeededError("needs to be respawned");
  }
}

//#region Player death with push away

const killWithPush = (act: Actor): void => {
  gPlayerAllowAnimations?.push(AnimationEventName.ragdoll);
  act.pushActorAway(act, 0);
}
const ressurectWithPushKill = (act: Actor): void => {
  act.forceRemoveRagdollFromWorld().then(() =>
    once("update", () => Debug.sendAnimationEvent(act, AnimationEventName.get_up_begin))
  );
};

//#endregion

//#region Player death with animation (fallback, second variant)

const killWithAnim = (act: Actor): void => {
  gPlayerAllowAnimations?.push(AnimationEventName.wound_default);
  once("update", () => {
    Debug.sendAnimationEvent(act, AnimationEventName.wound_default);
    act.forceRemoveRagdollFromWorld();
  });
}
const ressurectWithAnimKill = (act: Actor): void => {
  once("update", () => Debug.sendAnimationEvent(act, AnimationEventName.force_default));
}

//#endregion
