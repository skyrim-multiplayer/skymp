import { ChatMessage, createSystemMessage } from "../../../props/chatProperty";
import { Mp, ServerSettings } from "../../../types/mp";
import { PlayerController } from "../../PlayerController";
import { Command, HandlerInput } from "./command";

export class KickCommand extends Command {
    constructor(mp: Mp, controller: PlayerController) {
        super(mp, controller, "kick");
        this.serverSettings = this.mp.getServerSettings();
    }

    handle(input: HandlerInput): void {
        const { actorId } = input;
        const args = input.argsRaw?.split(' ') || [];

        // TODO: dehardcode this
        const adminMasterApiIds = [479, 485, 486, 487, 488, 489, 497];
        if (!adminMasterApiIds.includes(this.controller.getProfileId(actorId))) {
          this.controller.sendChatMessage(actorId, createSystemMessage('No permission'));
          return;
        }
        if (!input.argsRaw) {
          this.controller.sendChatMessage(actorId, createSystemMessage('Expected profile id as an argument'));
          return;
        }
        const targetMasterApiId = parseInt(input.argsRaw);
        for (const targetPlayerActorId of this.controller.getOnlinePlayers()) {
          if (this.controller.getProfileId(targetPlayerActorId) === targetMasterApiId) {
            // TODO: disable actor instead of quitGame which is local
            this.controller.quitGame(targetPlayerActorId);
            this.controller.sendChatMessage(actorId, createSystemMessage(`Kicked actor ${targetPlayerActorId.toString(16)}`));
            return;
          }
        }
        this.controller.sendChatMessage(actorId, createSystemMessage('Not found'));
    }

    private getDiscordRoles(actorId: number): string[] {
        return this.mp.get(actorId, 'private.discordRoles') as string[];
    }

    private serverSettings: ServerSettings;
}
