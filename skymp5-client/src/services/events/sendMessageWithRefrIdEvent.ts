import { RefrIdMessageBase } from "../messages/refrIdMessageBase";
import { SendMessageEvent } from "./sendMessageEvent";

// This was created for sending UpdateMovement and other updates for hosted NPCs
// '_refrId' will be turned into 'idx' before sending to the server
// In case of undefined '_refrId', we treat message as related to the local player, not an NPC
export type MessageWithRefrId<Message> = Omit<Message, "idx"> & RefrIdMessageBase;

export type SendMessageWithRefrIdEvent<Message> = SendMessageEvent<MessageWithRefrId<Message>>;
