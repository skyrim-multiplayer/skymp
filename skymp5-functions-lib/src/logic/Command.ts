import { PlayerController } from "./PlayerController";

export interface Command {
    name: string 
    handler: (actorId: number, controller: PlayerController) => void; 
    args?: string
}
