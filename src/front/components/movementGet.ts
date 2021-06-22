import { ObjectReference, Actor, TESModPlatform } from "skyrimPlatform";
import { NiPoint3 } from "../../lib/structures/movement";
import { Movement, RunMode } from "./movement";

export const getMovement = (refr: ObjectReference): Movement => {
  const ac = Actor.from(refr);

  // It is running for ObjectReferences because Standing
  // Doesn't lead to translateTo call
  const runMode = ac ? getRunMode(ac) : "Running";

  let healthPercentage = ac && ac.getActorValuePercentage("health");
  if (ac && ac.isDead()) {
    healthPercentage = 0;
  }

  let lookAt: undefined | NiPoint3 = undefined;
  if (ac.getFormID() !== 0x14) {
    const combatTarget = ac.getCombatTarget();
    if (combatTarget) {
      lookAt = [
        combatTarget.getPositionX(),
        combatTarget.getPositionY(),
        combatTarget.getPositionZ(),
      ];
    }
  }

  return {
    worldOrCell: (refr.getWorldSpace() || refr.getParentCell()).getFormID(),
    pos: [refr.getPositionX(), refr.getPositionY(), refr.getPositionZ()],
    rot: [refr.getAngleX(), refr.getAngleY(), refr.getAngleZ()],
    runMode: runMode,
    direction:
      runMode !== "Standing"
        ? 360 * refr.getAnimationVariableFloat("Direction")
        : 0,
    isInJumpState: ac && ac.getAnimationVariableBool("bInJumpState"),
    isSneaking: ac && isSneaking(ac),
    isBlocking: ac && ac.getAnimationVariableBool("IsBlocking"),
    isWeapDrawn: ac && ac.isWeaponDrawn(),
    healthPercentage,
    lookAt,
  };
};

const isSneaking = (ac: Actor) =>
  ac.isSneaking() || ac.getAnimationVariableBool("IsSneaking");

const getRunMode = (ac: Actor): RunMode => {
  if (ac.isSprinting()) return "Sprinting";

  const speed = ac.getAnimationVariableFloat("SpeedSampled");
  if (!speed) return "Standing";

  let isRunning = true;
  if (ac.getFormID() == 0x14) {
    if (!TESModPlatform.isPlayerRunningEnabled() || speed < 150)
      isRunning = false;
  } else {
    if (!ac.isRunning() || speed < 150) isRunning = false;
  }

  if (ac.getAnimationVariableFloat("IsBlocking")) {
    isRunning = isSneaking(ac);
  }

  const carryWeight = ac.getActorValue("CarryWeight");
  const totalItemWeight = ac.getTotalItemWeight();
  if (carryWeight < totalItemWeight) isRunning = false;

  return isRunning ? "Running" : "Walking";
};
