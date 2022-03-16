import { FormModel } from './model';
import { ObjectReference, Actor, TESModPlatform } from "skyrimPlatform";
import { NiPoint3, Movement, RunMode } from "./movement";
import { getDistance, getPos } from './movementApply';

class PlayerCharacterSpeedCalculator {
  static savePosition(pos: NiPoint3) {
    this.lastPcPos = pos;
    this.lastPcPosCheck = Date.now();
  }

  static getSpeed(currentPos: NiPoint3) {
    if (this.lastPcPosCheck === -1) return 0;

    const timeDeltaSec = (Date.now() - this.lastPcPosCheck) / 1000;
    if (timeDeltaSec > 5) return 0; // Too inaccurate
    if (timeDeltaSec === 0) return 0; // Division by zero

    const distance = getDistance(currentPos, this.lastPcPos);
    return distance / timeDeltaSec;
  }

  private static lastPcPos = [0, 0, 0];
  private static lastPcPosCheck = -1;
}

export const getMovement = (refr: ObjectReference, form?: FormModel): Movement => {
  const ac = Actor.from(refr);

  // It is running for ObjectReferences because Standing
  // Doesn't lead to translateTo call
  const runMode = ac ? getRunMode(ac) : "Running";

  let healthPercentage = ac && ac.getActorValuePercentage("health");
  if (ac && ac.isDead()) {
    healthPercentage = 0;
  }

  let lookAt: undefined | NiPoint3 = undefined;
  if (refr.getFormID() !== 0x14) {
    const combatTarget = ac?.getCombatTarget();
    if (combatTarget) {
      lookAt = [
        combatTarget.getPositionX(),
        combatTarget.getPositionY(),
        combatTarget.getPositionZ(),
      ];
    }
  }

  const pos = getPos(refr);

  let speed;
  if (refr.getFormID() !== 0x14) {
    speed = refr.getAnimationVariableFloat("SpeedSampled");
  }
  else {
    speed = PlayerCharacterSpeedCalculator.getSpeed(pos);
    PlayerCharacterSpeedCalculator.savePosition(pos);
  }

  const worldOrCell = refr.getWorldSpace() || refr.getParentCell();

  return {
    worldOrCell: worldOrCell?.getFormID() || 0,
    pos,
    rot: [refr.getAngleX(), refr.getAngleY(), refr.getAngleZ()],
    runMode: runMode,
    direction: runMode !== "Standing"
      ? 360 * refr.getAnimationVariableFloat("Direction")
      : 0,
    isInJumpState: !!(ac && ac.getAnimationVariableBool("bInJumpState")),
    isSneaking: !!(ac && isSneaking(ac)),
    isBlocking: !!(ac && ac.getAnimationVariableBool("IsBlocking")),
    isWeapDrawn: !!(ac && ac.isWeaponDrawn()),
    isDead: form?.isDead ?? false,
    healthPercentage: healthPercentage || 0,
    lookAt,
    speed
  };
}

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
