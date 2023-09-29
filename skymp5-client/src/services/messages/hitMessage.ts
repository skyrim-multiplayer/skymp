import { MsgType } from "../../messages";

export interface Hit {
    aggressor: number;
    isBashAttack: boolean;
    isHitBlocked: boolean;
    isPowerAttack: boolean;
    isSneakAttack: boolean;
    projectile: number;
    source: number;
    target: number;
}

export interface HitMessage {
    t: MsgType.OnHit,
    data: Hit
}
