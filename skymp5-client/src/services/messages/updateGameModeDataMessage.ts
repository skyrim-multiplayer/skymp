export interface UpdateGamemodeDataMessage {
    type: "updateGamemodeData";
    eventSources: Record<string, string>;
    updateOwnerFunctions: Record<string, string>;
    updateNeighborFunctions: Record<string, string>;
}
