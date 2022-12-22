import { ChatMessage, ChatNeighbor } from "../props/chatProperty";

export interface GameModeListener {
  onPlayerJoin?: (actorId: number) => void;
  onPlayerLeave?: (actorId: number) => void;
  onPlayerDeath?: (targetActorId: number, killerActorId?: number) => void;
  everySecond?: () => void;
  onPlayerChatInput?: (actorId: number, input: string, neighbors: number[], masterApiId: number) => void;
  onPlayerDialogResponse?: (actorId: number, dialogId: number, buttonIndex: number) => void;
  onPlayerActivateObject?: (casterActorId: number, targetObjectDesc: string, targetActorId: number) => 'continue' | 'blockActivation';
}
