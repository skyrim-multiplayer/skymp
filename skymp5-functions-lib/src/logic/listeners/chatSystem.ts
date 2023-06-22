import { ChatMessage } from "../../props/chatProperty";
import { Mp } from "../../types/mp";
import { PlayerController } from "../PlayerController";
import { GameModeListener } from "./gameModeListener";

export class ChatSystem implements GameModeListener {
    constructor(private mp: Mp, private controller: PlayerController) {
    }

    onPlayerChatInput(actorId: number, input: string, neighbors: number[], masterApiId: number): 'eventBusStop' {
        const lastChatMessageDate = this.mp.get(actorId, "private.lastChatMessageDate") || 0;
        if (Date.now() - lastChatMessageDate < 5000) {
            this.controller.sendChatMessage(actorId, ChatMessage.system("Сообщения можно отправлять не чаще чем раз в 5 секунд"));
            return 'eventBusStop';
        }
        this.mp.set(actorId, "private.lastChatMessageDate", Date.now());

        const message = new ChatMessage(actorId, masterApiId, input, 'plain', this.controller)
        for (const neighbor of neighbors) {
            this.controller.sendChatMessage(neighbor, message);
        }
        return 'eventBusStop';
    }
}
