import { PlayerController } from "./PlayerController";

export interface Command {
    name: string 
    handler: (actorId: number, controller: PlayerController, neighbors: number[], senderName: string, inputText: string) => void; 
    args?: string
}
