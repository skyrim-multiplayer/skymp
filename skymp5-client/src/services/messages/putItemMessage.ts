import { Extra } from "skyrimPlatform";
import { MsgType } from "../../messages";

export type PutItemMessage = {
    t: MsgType.PutItem,
    count: number;
    target: number;
} & Extra;
