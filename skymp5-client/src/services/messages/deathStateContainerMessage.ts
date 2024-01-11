
import { MsgType } from "../../messages";
import { ChangeValuesMessage } from "./changeValues";
import { TeleportMessage } from "./teleportMessage";
import { UpdatePropertyMessage } from "./updatePropertyMessage";

export interface DeathStateContainerMessage {
    t: MsgType.DeathStateContainer;
    tTeleport?: TeleportMessage,
    tChangeValues?: ChangeValuesMessage,
    tIsDead: UpdatePropertyMessage,
}
