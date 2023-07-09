import { createSystemMessage } from "../../../props/chatProperty";
import { Mp } from "../../../types/mp";
import { CombinedController, PlayerController } from "../../PlayerController";
import { Command, HandlerInput } from "./command";

export class KillCommand extends Command {
    constructor(mp: Mp, controller: CombinedController) {
        super(mp, controller, "kill");
    }

    handle(input: HandlerInput): void {
        const { actorId } = input;
        this.controller.setPercentages(actorId, { health: 0 });
        this.controller.sendChatMessage(actorId, createSystemMessage('You killed yourself...'));
    }
}
