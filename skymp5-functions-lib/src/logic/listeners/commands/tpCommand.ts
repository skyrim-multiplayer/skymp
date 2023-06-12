import { ChatMessage } from "../../../props/chatProperty";
import { Mp, LocationalData, ServerSettings } from "../../../types/mp";
import { PlayerController } from "../../PlayerController";
import { Command, HandlerInput } from "./command";

interface TeleportData {
    [key: string]: LocationalData;
}

export class TpCommand extends Command {
    private serverSettings: ServerSettings;
    private teleports: TeleportData;

    constructor(mp: Mp, controller: PlayerController) {
        super(mp, controller, "tp");
        this.serverSettings = this.mp.getServerSettings();
        this.teleports = {};
    }

    handle(input: HandlerInput): void {
        const { actorId } = input;
        const args = input.argsRaw?.split(' ') || [];
        const command = args[0];
        const teleportName = args[1];
    
        if (!command) {
            this.printHelp(actorId);
            return;
        }
    
        switch (command) {
            case 'create':
                this.createTeleport(actorId, teleportName);
                break;
            case 'list':
                this.listTeleports(actorId);
                break;
            case 'delete':
                this.deleteTeleport(actorId, teleportName);
                break;
            default:
                this.teleportPlayer(actorId, command); // command here is the teleport name
        }
    }
    
    printHelp(actorId: number): void {
        const helpMessage = [
            "/tp <teleport_name> - moves player to the saved location named teleport_name.",
            "/tp create <teleport_name> - creates a new teleport with name teleport_name in the player's current location.",
            "/tp list - lists all created teleports.",
            "/tp delete <teleport_name> - deletes the teleport with name teleport_name."
        ].join('\n');
    
        this.controller.sendChatMessage(actorId, ChatMessage.system(helpMessage));
    }
    

    createTeleport(actorId: number, teleportName: string): void {
        const locationalData = this.mp.get(actorId, 'locationalData') as LocationalData;
        this.teleports[teleportName] = locationalData;
        this.controller.sendChatMessage(actorId, ChatMessage.system(`Teleport location ${teleportName} has been created.`));
    }

    listTeleports(actorId: number): void {
        const teleportNames = Object.keys(this.teleports).join(', ');
        this.controller.sendChatMessage(actorId, ChatMessage.system(`Teleports: ${teleportNames}`));
    }

    deleteTeleport(actorId: number, teleportName: string): void {
        delete this.teleports[teleportName];
        this.controller.sendChatMessage(actorId, ChatMessage.system(`Teleport location ${teleportName} has been deleted.`));
    }

    teleportPlayer(actorId: number, teleportName: string): void {
        const teleportData = this.teleports[teleportName];
        if (teleportData) {
            this.mp.set(actorId, 'locationalData', teleportData);
            this.controller.sendChatMessage(actorId, ChatMessage.system(`Teleported to ${teleportName}.`));
        } else {
            this.controller.sendChatMessage(actorId, ChatMessage.system(`No teleport location found with name ${teleportName}.`));
        }
    }

    getTeleports(): TeleportData {
        return this.teleports;
    }

    setTeleports(data: TeleportData): void {
        this.teleports = data;
    }
}
