import { ChatMessage, createSystemMessage } from "../../props/chatProperty";
import { Mp, ServerSettings } from "../../types/mp";
import { PlayerController } from "../PlayerController";
import { GameModeListener } from "./gameModeListener";

export class ConsoleCommandsSystem implements GameModeListener {
    constructor(private mp: Mp, private controller: PlayerController) {
        this.serverSettings = this.mp.getServerSettings();
    }

    onPlayerJoin(actorId: number) {
        const role = this.serverSettings.discordAuth?.adminRoleId;
        const discordRoles = this.getDiscordRoles(actorId);

        if (role === undefined) {
            console.warn("Admin role is not configured. discordAuth.adminRoleId should be set in server-settings.json");
            return;
        }

        const consoleCommandsAllowed = discordRoles.includes(role);

        this.mp.set(actorId, "consoleCommandsAllowed", consoleCommandsAllowed);
    }

    private getDiscordRoles(actorId: number): string[] {
        return this.mp.get(actorId, 'private.discordRoles') as string[];
    }

    private serverSettings: ServerSettings;
}
