import { ChatMessage } from "../../../props/chatProperty";
import { Mp } from "../../../types/mp";
import { PlayerController } from "../../PlayerController";
import { GameModeListener } from "../GameModeListener";

export class KitsSystem implements GameModeListener {
    constructor(private mp: Mp, private controller: PlayerController) {
    }

    onPlayerChatInput(actorId: number, input: string, neighbors: number[], masterApiId: number) {
        if (input === '/kit base') {
            const discordRoles = this.getDiscordRoles(actorId);
            console.log({discordRoles})
            if (discordRoles.includes("1106647876022583381")) {
                this.controller.sendChatMessage(actorId, ChatMessage.system("You have been given a base kit", this.controller));
                this.controller.addItem(actorId, 0xf, 1000);
                return 'eventBusStop';
            }
            else {
                this.controller.sendChatMessage(actorId, ChatMessage.system("You do not have permission to use this kit", this.controller));
                return 'eventBusStop';
            }
        }
        else if (input.startsWith(`/kit `) || input === '/kit') {
            this.controller.sendChatMessage(actorId, ChatMessage.system("/kit <name>", this.controller));
            this.controller.sendChatMessage(actorId, ChatMessage.system("Example: /kit base", this.controller));
            return 'eventBusStop';
        }
        return 'eventBusContinue';
    }

    private getDiscordRoles(actorId: number): string[] {
        try {
            this.mp.makeProperty("discordRoles", {
              isVisibleByNeighbors: false,
              isVisibleByOwner: false,
              updateNeighbor: "",
              updateOwner: ""
            });
          }
          catch (e) {
            if (`${e}`.indexOf("must be unique") === -1) {
              throw e;
            }
        }
        return this.mp.get(actorId, 'discordRoles') as string[];
    }
}
