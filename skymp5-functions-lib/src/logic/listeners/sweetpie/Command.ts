import { ChatNeighbor } from "../../../props/chatProperty";
import { PlayerController } from "../../PlayerController";

export interface HandlerInput {
  actorId: number;
  controller: PlayerController;
  neighbors: number[];
  inputText: string;
  argsRaw?: string;
  senderName?: string;
  masterApiId?: number;
}

export interface Command {
  name: string;
  handler: (input: HandlerInput) => void;
}
