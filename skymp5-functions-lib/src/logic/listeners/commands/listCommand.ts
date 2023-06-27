import { createSystemMessage } from "../../../props/chatProperty";
import { Mp } from "../../../types/mp";
import { PlayerController } from "../../PlayerController";
import { Command, HandlerInput } from "./command";

export class ListCommand extends Command {
    constructor(mp: Mp, controller: PlayerController) {
        super(mp, controller, "list");
    }

    handle(input: HandlerInput): void {
        const { actorId } = input;
        const data = this.controller.getOnlinePlayers()
          .map((playerFormId) => ({
            name: this.controller.getName(playerFormId),
            ids: `${playerFormId.toString(16)}/${this.controller.getProfileId(playerFormId)}`,
          }))
          .filter(({ name }) => name.toLocaleLowerCase().indexOf(input.argsRaw?.toLocaleLowerCase() ?? '') !== -1)
          .sort((a, b) => a.name.toLocaleLowerCase().localeCompare(b.name.toLocaleLowerCase()));
        this.controller.sendChatMessage(actorId, createSystemMessage(`${data.length} players ${input.argsRaw ? 'matched' : 'online'}: Server ID / Master API ID - Name`));
        for (const { name, ids } of data) {
          this.controller.sendChatMessage(actorId, createSystemMessage(`${ids} - ${name}`));
        }
    }
}
