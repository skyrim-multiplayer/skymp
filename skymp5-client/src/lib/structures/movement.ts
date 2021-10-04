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
  healthPercentage: number;
  lookAt?: NiPoint3;
}

export type Movement = Transform & AnimationVariables;
