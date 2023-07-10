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

        const role = this.serverSettings.discordAuth?.adminRoleId;
        const discordRoles = this.getDiscordRoles(actorId);
        if (role === undefined) {
          this.controller.sendChatMessage(actorId, ChatMessage.system("Admin role is not configured.", this.controller));
          this.controller.sendChatMessage(actorId, ChatMessage.system("discordAuth.adminRoleId should be set in server-settings.json", this.controller));
        }
        else if (discordRoles.includes(role)) {
          if (!input.argsRaw) {
            this.controller.sendChatMessage(actorId, createSystemMessage('Expected profile id as an argument'));
            return;
          }
          const targetMasterApiId = parseInt(input.argsRaw);
          for (const targetPlayerActorId of this.controller.getOnlinePlayers()) {
            if (this.controller.getProfileId(targetPlayerActorId) === targetMasterApiId) {
              this.controller.sendChatMessage(actorId, createSystemMessage(`Kicked actor ${targetPlayerActorId.toString(16)}`));
              this.controller.sendChatMessage(targetPlayerActorId, createSystemMessage('You have been kicked. Exiting game in 5 seconds.'));
              setTimeout(() => {
                // Quits game unsafe (player can resist)
                this.controller.quitGame(targetPlayerActorId);

                // Quits to main menu safely by disabling the actor
                // TODO: add setEnabled and other ScampServer APIs to types
                (this.mp.setEnabled as (actorId: number, set: boolean) => void) (targetPlayerActorId, false);

                // TODO: Kick instead of crashing the game?
                // TODO: Teleport far away to prevent instant reconnecting to the same place?
              }, 5000);
              return;
            }
          }
          this.controller.sendChatMessage(actorId, createSystemMessage('Not found'));
        }
        else {
          console.log(discordRoles);
          this.controller.sendChatMessage(actorId, ChatMessage.system("You do not have permission to use this command", this.controller));
        }
    }

    private getDiscordRoles(actorId: number): string[] {
        return this.mp.get(actorId, 'private.discordRoles') as string[];
    }

    private serverSettings: ServerSettings;
}
