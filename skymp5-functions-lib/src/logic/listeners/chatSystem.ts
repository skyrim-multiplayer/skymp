import { ChatMessage } from "../../props/chatProperty";
import { PlayerController } from "../PlayerController";
import { GameModeListener } from "./gameModeListener";

export class ChatSystem implements GameModeListener {
    constructor(private controller: PlayerController) {
    }

    onPlayerChatInput(actorId: number, input: string, neighbors: number[], masterApiId: number): 'eventBusStop' {
        const message = new ChatMessage(actorId, masterApiId, input, 'plain', this.controller)
        for (const neighbor of neighbors) {
            this.controller.sendChatMessage(neighbor, message);
        }
        return 'eventBusStop';
    }
}
