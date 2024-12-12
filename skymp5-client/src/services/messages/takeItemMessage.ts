import { Extra } from "skyrimPlatform";
import { MsgType } from "../../messages";

export interface TakeItemMessage extends Extra {
    t: MsgType.TakeItem,
    baseId: number;
    count: number;
    target: number;
};
