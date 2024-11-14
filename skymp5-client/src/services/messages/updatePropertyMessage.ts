import { MsgType } from "../../messages";

export interface UpdatePropertyMessage {
    t: MsgType.UpdateProperty;
    idx: number;
    refrId: number;
    baseRecordType: string; // DOOR, ACTI, etc
    dataDump?: string; // JSON string, alternatives with 'data'
    data?: unknown; // JSON value, alternatives with 'dataDump'
    propName: string;
}
