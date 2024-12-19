import { MsgType } from "../../messages";

export interface GamemodeValuePair {
    name: string;
    content: string;
}

export interface UpdateGamemodeDataMessage {
    t: MsgType.UpdateGamemodeData;
    eventSources: GamemodeValuePair[];
    updateOwnerFunctions: GamemodeValuePair[];
    updateNeighborFunctions: GamemodeValuePair[];
}
