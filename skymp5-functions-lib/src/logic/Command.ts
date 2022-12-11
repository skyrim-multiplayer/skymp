import { ChatNeighbor } from "../props/chatProperty";
import { PlayerController } from "./PlayerController";

export interface HandlerInput {
  actorId: number;
  controller: PlayerController;
  neighbors: ChatNeighbor[];
  senderName: string;
  inputText: string;
  argsRaw?: string;
}

export interface Command {
  name: string;
  handler: (input: HandlerInput) => void;
}
