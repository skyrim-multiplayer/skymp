import { Mp } from "../../../types/mp";
import { PlayerController } from "../../PlayerController";
import { GameModeListener } from "../gameModeListener";

export interface HandlerInput {
  actorId: number;
  controller: PlayerController;
  mp: Mp;
  neighbors: number[];
  inputText: string;
  argsRaw?: string;
  senderName?: string;
  masterApiId?: number;
}

// Base class for all chat commands
export abstract class Command implements GameModeListener {
  constructor(protected mp: Mp, protected controller: PlayerController, protected name: string) {
  }

  onPlayerChatInput(actorId: number, input: string, neighbors: number[], masterApiId: number) {
    if (input === '/' + this.name || input.startsWith(`/${this.name} `)) {
      this.handle({ actorId, mp: this.mp, controller: this.controller, neighbors, masterApiId, inputText: input, argsRaw: input.substring(this.name.length + 2) });
      return 'eventBusStop';
    }
    return 'eventBusContinue';
  }

  abstract handle(input: HandlerInput): void;
}
