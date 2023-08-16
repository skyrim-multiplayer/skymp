import { MsgType } from "../../messages";

export interface CustomEventMessage {
    t: MsgType.CustomEvent,
    args: unknown[],
    eventName: string
}
