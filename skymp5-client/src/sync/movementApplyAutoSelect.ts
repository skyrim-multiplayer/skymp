import { ObjectReference, settings } from "skyrimPlatform";
import { Movement } from "./movement";
import { applyMovement } from "./movementApply";
import { applyMovementNg } from "./movementApplyNg";


export const applyMovementAutoSelect = (refr: ObjectReference, m: Movement, isMyClone?: boolean): void => {
    applyMovement(refr, m, isMyClone);
    // settings["skymp5-client"]["use-old-movement"]
    //     ? applyMovement(refr, m, isMyClone)
    //     : applyMovementNg(refr, m, isMyClone);
}
