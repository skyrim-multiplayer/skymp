export type NiPoint3 = [number, number, number];

export interface Transform {
  worldOrCell: number;
  pos: NiPoint3;
  rot: NiPoint3;
}

export type RunMode = "Standing" | "Walking" | "Running" | "Sprinting";

export interface AnimationVariables {
  runMode: RunMode;
  direction: number;
  isInJumpState: boolean;
  isSneaking: boolean;
  isBlocking: boolean;
  isWeapDrawn: boolean;
  isDead: boolean;
  healthPercentage: number;
  lookAt?: NiPoint3;
  speed: number;
}

export type Movement = Transform & AnimationVariables;

// // This doesn't work in SkyrimPlatform 0.5.0:
// export * from './movementApply';
// export * from './movementGet';

// Temporary workaround:
import * as movementApply from "./movementApply";
export const applyMovement = movementApply.applyMovement;
import * as movementGet from "./movementGet";
export const getMovement = movementGet.getMovement;
