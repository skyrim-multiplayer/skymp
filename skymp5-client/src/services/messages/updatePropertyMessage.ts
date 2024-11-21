import { MsgType } from "../../messages";

export interface UpdatePropertyMessage {
    t: MsgType.UpdateProperty;
    idx: number;
    refrId: number;
    baseRecordType: string; // DOOR, ACTI, etc
    data: unknown;
    propName: string;
}
