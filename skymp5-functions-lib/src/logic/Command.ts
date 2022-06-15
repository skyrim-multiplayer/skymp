import { PlayerController } from "./PlayerController";

export interface HandlerInput {
  actorId: number;
  controller: PlayerController;
  argsRaw?: string;
}

export interface Command {
  name: string;
  handler: (input: HandlerInput) => void;
}
