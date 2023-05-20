import { ChatMessage } from "../../../props/chatProperty";
import { Mp, ServerSettings } from "../../../types/mp";
import { PlayerController } from "../../PlayerController";
import { Command, HandlerInput } from "./command";

export class KitCommand extends Command {
    constructor(mp: Mp, controller: PlayerController) {
        super(mp, controller, "kit");
        this.serverSettings = this.mp.getServerSettings();
    }

    handle(input: HandlerInput): void {
        const { actorId } = input;
        const args = input.argsRaw?.split(' ') || [];

        const kitName = args[0];
        if (kitName === 'base') {
            const baseKitDiscordRole = this.serverSettings.discordAuth?.baseKitRoleId;
            const discordRoles = this.getDiscordRoles(actorId);
            if (baseKitDiscordRole === undefined) {
                this.controller.sendChatMessage(actorId, ChatMessage.system("Base kit is not configured.", this.controller));
                this.controller.sendChatMessage(actorId, ChatMessage.system("discordAuth.baseKitRoleId should be set in server-settings.json", this.controller));
            }
            else if (discordRoles.includes(baseKitDiscordRole)) {
                const previousDate = this.mp.get(actorId, "private.kits.lastBaseKitGiftDate") as number | undefined;
                const currentDate = Date.now();
                const days = 1;
                const secondsBetweenGifts = 60 * 60 * 24 * days;
                if (!previousDate || currentDate - previousDate > secondsBetweenGifts * 1000) {
                    this.controller.sendChatMessage(actorId, ChatMessage.system("You have been given a base kit", this.controller));
                    this.controller.addItem(actorId, 0xf, 1000);
                    this.mp.set(actorId, "private.kits.lastBaseKitGiftDate", currentDate);
                }
                else {
                    const diffSeconds = secondsBetweenGifts - Math.ceil((currentDate - previousDate) / 1000);
                    this.controller.sendChatMessage(actorId, ChatMessage.system(`${diffSeconds} seconds remaining`, this.controller));
                }
            }
            else {
                this.controller.sendChatMessage(actorId, ChatMessage.system("You do not have permission to use this kit", this.controller));
            }
        }

        this.controller.sendChatMessage(input.actorId, ChatMessage.system("/kit <name>", this.controller));
        this.controller.sendChatMessage(input.actorId, ChatMessage.system("Example: /kit base", this.controller));
    }

    private getDiscordRoles(actorId: number): string[] {
        return this.mp.get(actorId, 'private.discordRoles') as string[];
    }

    private serverSettings: ServerSettings;
}
