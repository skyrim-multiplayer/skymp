import { MsgType } from "../../messages";
import { Animation } from "../../sync/animation";

export interface UpdateAnimationMessage {
    t: MsgType.UpdateAnimation;
    idx: number;
    data: Animation;
}
