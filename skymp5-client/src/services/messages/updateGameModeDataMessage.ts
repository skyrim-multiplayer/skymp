export interface GamemodeValuePair {
    name: string;
    content: string;
}

export interface UpdateGamemodeDataMessage {
    type: "updateGamemodeData";
    eventSources: GamemodeValuePair[];
    updateOwnerFunctions: GamemodeValuePair[];
    updateNeighborFunctions: GamemodeValuePair[];
}
