import { Extra } from "skyrimPlatform";
import { MsgType } from "../../messages";

export type TakeItemMessage = {
    t: MsgType.TakeItem,
    count: number;
    target: number;
} & Extra;
