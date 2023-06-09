import { ChatMessage } from "../../../props/chatProperty";
import { Mp, LocationalData, ServerSettings } from "../../../types/mp";
import { PersistentStorage } from "../../../utils/persistentStorage";
import { PlayerController } from "../../PlayerController";
import { Command, HandlerInput } from "./command";

export class TpCommand extends Command {
    private serverSettings: ServerSettings;
    private persistentStorage: PersistentStorage;

    constructor(mp: Mp, controller: PlayerController) {
        super(mp, controller, "tp");
        this.serverSettings = this.mp.getServerSettings();
        this.persistentStorage = PersistentStorage.getSingleton();
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
        const teleports = this.persistentStorage.teleports;
        teleports[teleportName] = locationalData;
        this.persistentStorage.teleports = teleports;
        this.controller.sendChatMessage(actorId, ChatMessage.system(`Teleport location ${teleportName} has been created.`));
    }

    listTeleports(actorId: number): void {
        const teleports = this.persistentStorage.teleports;
        const teleportNames = Object.keys(teleports).join(', ');
        this.controller.sendChatMessage(actorId, ChatMessage.system(`Teleports: ${teleportNames}`));
    }

    deleteTeleport(actorId: number, teleportName: string): void {
        const teleports = this.persistentStorage.teleports;
        delete teleports[teleportName];
        this.persistentStorage.teleports = teleports;
        this.controller.sendChatMessage(actorId, ChatMessage.system(`Teleport location ${teleportName} has been deleted.`));
    }

    teleportPlayer(actorId: number, teleportName: string): void {
        const teleports = this.persistentStorage.teleports;
        const teleportData = teleports[teleportName];
        if (teleportData) {
            this.mp.set(actorId, 'locationalData', teleportData);
            this.controller.sendChatMessage(actorId, ChatMessage.system(`Teleported to ${teleportName}.`));
        } else {
            this.controller.sendChatMessage(actorId, ChatMessage.system(`No teleport location found with name ${teleportName}.`));
        }
    }
}
