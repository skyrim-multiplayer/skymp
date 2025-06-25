import { MsgType } from "../../messages";

export interface DestroyActorMessage {
    t: MsgType.DestroyActor,
    idx: number;
}
