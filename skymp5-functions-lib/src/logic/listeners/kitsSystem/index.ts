import { ChatMessage } from "../../../props/chatProperty";
import { Mp, ServerSettings } from "../../../types/mp";
import { PlayerController } from "../../PlayerController";
import { GameModeListener } from "../GameModeListener";

export class KitsSystem implements GameModeListener {
    constructor(private mp: Mp, private controller: PlayerController) {
        this.serverSettings = this.mp.getServerSettings();
    }

    onPlayerChatInput(actorId: number, input: string, neighbors: number[], masterApiId: number) {
        const baseKitDiscordRole = this.serverSettings.discordAuth?.baseKitRoleId;

        if (input === '/kit base') {
            const discordRoles = this.getDiscordRoles(actorId);
            if (baseKitDiscordRole === undefined) {
                this.controller.sendChatMessage(actorId, ChatMessage.system("Base kit is not configured.", this.controller));
                this.controller.sendChatMessage(actorId, ChatMessage.system("discordAuth.baseKitRoleId should be set in server-settings.json", this.controller));
                return 'eventBusStop';
            }
            else if (discordRoles.includes(baseKitDiscordRole)) {
                const previousDate = this.mp.get(actorId, "private.kits.lastBaseKitGiftDate") as number | undefined;
                const currentDate = Date.now();
                const days = 1;
                const secondsBetweenGifts = 60 * 60 * 24 * days;
                if (!previousDate || currentDate - previousDate > secondsBetweenGifts * 1000) {
                    console.log(currentDate - previousDate)
                    this.controller.sendChatMessage(actorId, ChatMessage.system("You have been given a base kit", this.controller));
                    this.controller.addItem(actorId, 0xf, 1000);
                    this.mp.set(actorId, "private.kits.lastBaseKitGiftDate", currentDate);
                }
                else {
                    const diffSeconds = secondsBetweenGifts - Math.ceil((currentDate - previousDate) / 1000);
                    this.controller.sendChatMessage(actorId, ChatMessage.system(`${diffSeconds} seconds remaining`, this.controller));
                }
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
        return this.mp.get(actorId, 'private.discordRoles') as string[];
    }

    private serverSettings: ServerSettings;
}
