import { MsgType } from "../../messages";
import { Movement } from "../../sync/movement";

export interface UpdateMovementMessage {
    t: MsgType.UpdateMovement;
    idx: number;
    data: Movement;
}
