import { MsgType } from "../../messages";

export interface SpellCastMsgData {
    caster: number
    target: number
    spell: number
    isDualCasting: boolean
    interruptCast: boolean
    castingSource: number
    booleanAnimationVariables: ArrayBuffer
    floatAnimationVariables: ArrayBuffer
    integerAnimationVariables: ArrayBuffer
}

export interface SpellCastMessage {
    t: MsgType.SpellCast,
    data: SpellCastMsgData
}
