import * as structures from "../../lib/structures/movement";
export type Movement = structures.Movement;
export type RunMode = structures.RunMode;
export type AnimationVariables = structures.AnimationVariables;
export type Transform = structures.Transform;
export type NiPoint3 = structures.NiPoint3;

// // This doesn't work in SkyrimPlatform 0.5.0:
// export * from './movementApply';
// export * from './movementGet';

// Temporary workaround:
import * as movementApply from "./movementApply";
export const applyMovement = movementApply.applyMovement;
import * as movementGet from "./movementGet";
export const getMovement = movementGet.getMovement;
