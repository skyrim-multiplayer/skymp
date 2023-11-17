import { MsgType } from "../../messages";

export interface OpenContainerMessage {
    t: MsgType.OpenContainer;
    target: number;
}
