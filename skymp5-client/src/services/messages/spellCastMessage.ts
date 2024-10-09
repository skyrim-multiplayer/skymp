import { MsgType } from "../../messages";
import { ActorAnimationVariables } from 'skyrimPlatform';

export interface SpellCastMsgData {
    caster: number
    target: number
    spell: number
    isDualCasting: boolean
    interruptCast: boolean
    castingSource: number
    actorAnimationVariables: ActorAnimationVariables
}

export interface SpellCastMessage {
    t: MsgType.SpellCast,
    data: SpellCastMsgData
}
