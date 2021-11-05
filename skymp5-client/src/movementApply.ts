import {
  ObjectReference,
  Actor,
  Game,
  TESModPlatform,
  Debug,
  Form,
} from "skyrimPlatform";
import { applyDeathState } from "./deathSystem";
import { RespawnNeededError } from "./errors";
import { Movement, RunMode, AnimationVariables, Transform } from "./movement";

export const applyMovement = (refr: ObjectReference, m: Movement): void => {
  if (teleportIfNeed(refr, m)) return;

  translateTo(refr, m);

  const ac = Actor.from(refr);

  if (ac) {
    let lookAt: Actor | null = undefined as unknown as Actor;
    if (m.lookAt) {
      try {
        lookAt = Game.findClosestActor(
          m.lookAt[0],
          m.lookAt[1],
          m.lookAt[2],
          128
        );
      } catch (e) {
        lookAt = null;
      }
    }

    if (lookAt as Actor) {
      ac.setHeadTracking(true);
      ac.setLookAt(lookAt, false);
    } else {
      ac.setHeadTracking(false);
    }

    // ac.stopCombat();
    ac.blockActivation(true);

    keepOffsetFromActor(ac, m);

    applySprinting(ac, m.runMode === "Sprinting");
    applyBlocking(ac, m);
    applySneaking(ac, m.isSneaking);
    applyWeapDrawn(ac, m.isWeapDrawn);
    applyHealthPercentage(ac, m.healthPercentage);
    applyDeathState(ac, m.isDead);
  }
};

const keepOffsetFromActor = (ac: Actor, m: Movement) => {
  let offsetAngle = m.rot[2] - ac.getAngleZ();
  if (Math.abs(offsetAngle) < 5) offsetAngle = 0;

  if (m.runMode === "Standing") {
    return ac.keepOffsetFromActor(ac, 0, 0, 0, 0, 0, offsetAngle, 1, 1);
  }
  const offset = [
    3 * Math.sin((m.direction / 180) * Math.PI),
    3 * Math.cos((m.direction / 180) * Math.PI),
    getOffsetZ(m.runMode),
  ];

  ac.keepOffsetFromActor(
    ac,
    offset[0],
    offset[1],
    offset[2],
    0,
    0,
    offsetAngle,
    m.runMode === "Walking" ? 2048 : 1,
    1
  );
};

const getOffsetZ = (runMode: RunMode) => {
  switch (runMode) {
    case "Walking":
      return -512;
    case "Running":
      return -1024;
  }
  return 0;
};

const applySprinting = (ac: Actor, isSprinting: boolean) => {
  if (ac.isSprinting() != isSprinting) {
    Debug.sendAnimationEvent(ac, isSprinting ? "SprintStart" : "SprintStop");
  }
};

const applyBlocking = (ac: Actor, m: AnimationVariables) => {
  if (ac.getAnimationVariableBool("IsBlocking") != m.isBlocking) {
    Debug.sendAnimationEvent(ac, m.isBlocking ? "BlockStart" : "BlockStop");
    Debug.sendAnimationEvent(ac, m.isSneaking ? "SneakStart" : "SneakStop");
  }
};

const applySneaking = (ac: Actor, isSneaking: boolean) => {
  const currentIsSneaking =
    ac.isSneaking() || ac.getAnimationVariableBool("IsSneaking");
  if (currentIsSneaking != isSneaking) {
    Debug.sendAnimationEvent(ac, isSneaking ? "SneakStart" : "SneakStop");
  }
};

export const applyWeapDrawn = (ac: Actor, isWeapDrawn: boolean): void => {
  if (ac.isWeaponDrawn() !== isWeapDrawn) {
    TESModPlatform.setWeaponDrawnMode(ac, isWeapDrawn ? 1 : 0);
  }
};

const applyHealthPercentage = (ac: Actor, healthPercentage: number) => {
  const currentPercentage = ac.getActorValuePercentage('health');
  if (currentPercentage === healthPercentage) return;

  const currentMax = ac.getBaseActorValue('health');
  const deltaPercentage = healthPercentage - currentPercentage;
  const k = 0.25;
  if (deltaPercentage > 0) {
    ac.restoreActorValue('health', deltaPercentage * currentMax * k);
  } else if (deltaPercentage < 0) {
    ac.damageActorValue('health', deltaPercentage * currentMax * k);
  }
};

const translateTo = (refr: ObjectReference, m: Movement) => {
  const distance = getDistance(getPos(refr), m.pos);
  let time = 0.1;
  if (m.isInJumpState) time = 0.2;
  if (m.runMode !== "Standing") time = 0.2;
  const speed = distance / time;

  const angleDiff = Math.abs(m.rot[2] - refr.getAngleZ());
  if (
    m.runMode != "Standing" ||
    m.isInJumpState ||
    distance > 64 ||
    angleDiff > 80
  ) {
    const actor = Actor.from(refr);
    if (actor && actor.getActorValue("Variable10") < -999) return;

    if (!actor || !actor.isDead()) {
      refr.translateTo(
        m.pos[0],
        m.pos[1],
        m.pos[2],
        m.rot[0],
        m.rot[1],
        m.rot[2],
        speed,
        0
      );
    }
  }
};

const teleportIfNeed = (refr: ObjectReference, m: Transform) => {
  if (
    isInDifferentWorldOrCell(refr, m.worldOrCell) ||
    (!refr.is3DLoaded() && isInDifferentExteriorCell(refr, m.pos))
  ) {
    throw new RespawnNeededError("needs to be respawned");
  }
  return false;
};

const cellWidth = 4096;

const isInDifferentExteriorCell = (refr: ObjectReference, pos: number[]) => {
  const currentPos = getPos(refr);
  const playerPos = getPos(Game.getPlayer() as Actor);
  const targetDistanceToPlayer = getDistance(playerPos, pos);
  const currentDistanceToPlayer = getDistance(playerPos, currentPos);
  return (
    currentDistanceToPlayer > cellWidth && targetDistanceToPlayer <= cellWidth
  );
};

const isInDifferentWorldOrCell = (
  refr: ObjectReference,
  worldOrCell: number
) => {
  return (
    worldOrCell !== ((refr.getWorldSpace() || refr.getParentCell()) as Form).getFormID()
  );
};

const getPos = (refr: ObjectReference) => {
  return [refr.getPositionX(), refr.getPositionY(), refr.getPositionZ()];
};

const getDistance = (a: number[], b: number[]) => {
  let r = 0;
  a.forEach((v, i) => (r += Math.pow(a[i] - b[i], 2)));
  return Math.sqrt(r);
};
