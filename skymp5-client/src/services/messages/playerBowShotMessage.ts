import { MsgType } from "../../messages";

export interface PlayerBowShotMessage {
    t: MsgType.PlayerBowShot,
    weaponId: number,
    ammoId: number,
    power: number,
    isSunGazing: boolean
};
