import { MsgType } from "../../messages";
import { ActorAnimationVariables } from 'skyrimPlatform';

export interface UpdateAnimVariablesMessageMsgData {
    actorRemoteId: number
    actorAnimationVariables: ActorAnimationVariables
}

export interface UpdateAnimVariablesMessage {
    t: MsgType.UpdateAnimVariables,
    data: UpdateAnimVariablesMessageMsgData
}
