import { MsgType } from "../../messages";

export interface SpellCastMsgData {
    caster: number
    target: number
    spell: number
    isDualCasting: boolean
    interruptCast: boolean
    castingSource: number
    aimAngle: number,
    aimHeading: number,
    actorAnimationVariables: {
        booleans: number[]
        floats: number[]
        integers: number[]
    }
}

export interface SpellCastMessage {
    t: MsgType.SpellCast,
    data: SpellCastMsgData
}
