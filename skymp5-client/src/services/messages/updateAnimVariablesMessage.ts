import { MsgType } from "../../messages";

export interface UpdateAnimVariablesMessageMsgData {
    actorRemoteId: number
    actorAnimationVariables: {
        booleans: number[]
        floats: number[]
        integers: number[]
    }
}

export interface UpdateAnimVariablesMessage {
    t: MsgType.UpdateAnimVariables,
    data: UpdateAnimVariablesMessageMsgData
}
