import { MsgType } from "../../messages";

export interface SetRaceMenuOpenMessage {
    t: MsgType.SetRaceMenuOpen;
    open: boolean;
}
