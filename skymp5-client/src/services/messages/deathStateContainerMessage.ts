import { MsgType, UpdatePropertyMessage } from "../../messages";
import { ChangeValuesMessage } from "./changeValues";
import { TeleportMessage } from "./teleportMessage";

export interface DeathStateContainerMessage {
    t: MsgType.DeathStateContainer;
    tTeleport?: TeleportMessage,
    tChangeValues?: ChangeValuesMessage,
    tIsDead: UpdatePropertyMessage,
}
