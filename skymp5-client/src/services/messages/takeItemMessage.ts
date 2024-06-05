import { Extra } from "skyrimPlatform";
import { MsgType } from "../../messages";

export interface TakeItemMessage extends Extra {
    t: MsgType.TakeItem,
    count: number;
    target: number;
};
