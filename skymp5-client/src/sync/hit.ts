import * as sp from "skyrimPlatform";
import { localIdToRemoteId } from "../view/worldViewMisc";

export interface Hit {
  aggressor: number;
  isBashAttack: boolean;
  isHitBlocked: boolean;
  isPowerAttack: boolean;
  isSneakAttack: boolean;
  projectile: number;
  source: number;
  target: number;
}

export const getHitData = (e: sp.HitEvent): Hit => {
  const hitData: Hit = {
    aggressor: localIdToRemoteId(e.aggressor.getFormID()),
    isBashAttack: e.isBashAttack,
    isHitBlocked: e.isHitBlocked,
    isPowerAttack: e.isPowerAttack,
    isSneakAttack: e.isSneakAttack,
    projectile: e.projectile ? e.projectile.getFormID() : 0,
    source: e.source ? e.source.getFormID() : 0,
    target: localIdToRemoteId(e.target.getFormID())
  }
  return hitData;
}
