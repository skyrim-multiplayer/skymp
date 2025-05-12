import { MsgType } from "../../messages";
import { Equipment } from "../../sync/equipment";

export interface UpdateVoiceChatMessage {
    t: MsgType.UpdateVoiceChatMessage;
    idx: number,
    data: { isTalking: boolean };
}
