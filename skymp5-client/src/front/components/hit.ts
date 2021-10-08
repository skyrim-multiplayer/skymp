import * as structures from "../../lib/structures/hit";
import * as sp from "skyrimPlatform";
import { localIdToRemoteId } from "../view";
export type Hit = structures.Hit;

export const getHitData = (e:sp.HitEvent): Hit => {
    const hitData: Hit = {
        agressor: localIdToRemoteId(e.agressor.getFormID()),
        isBashAttack: e.isBashAttack,
        isHitBlocked: e.isHitBlocked,
        isPowerAttack: e.isPowerAttack,
        isSneakAttack: e.isSneakAttack,
        projectile: e.projectile?.getFormID(),
        source: e.source?.getFormID(),
        target: localIdToRemoteId(e.target.getFormID())
    }
    return hitData;
}
