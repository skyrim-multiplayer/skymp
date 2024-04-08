import { MsgType } from "../../messages";
import { Appearance } from "../../sync/appearance";

export interface UpdateAppearanceMessage {
    t: MsgType.UpdateAppearance;
    idx: number;
    data: Appearance | null;
}
