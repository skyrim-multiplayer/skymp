import { MsgType } from "../../messages";

export interface OpenContainer {
    t: MsgType.OpenContainer;
    target: number;
}
