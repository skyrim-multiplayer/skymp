import { MsgType } from "../../messages";
// @ts-expect-error (TODO: Remove in 2.10.0)
import { ActorAnimationVariables } from 'skyrimPlatform';

export interface UpdateAnimVariablesMessageMsgData {
    actorRemoteId: number
    actorAnimationVariables: ActorAnimationVariables
}

export interface UpdateAnimVariablesMessage {
    t: MsgType.UpdateAnimVariables,
    data: UpdateAnimVariablesMessageMsgData
}
