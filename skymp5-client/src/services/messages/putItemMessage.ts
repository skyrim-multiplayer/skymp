import { Extra } from "skyrimPlatform";
import { MsgType } from "../../messages";

export interface PutItemMessage extends Extra {
    t: MsgType.PutItem,
    count: number;
    target: number;
};
