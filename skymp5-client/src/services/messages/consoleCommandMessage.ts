import { MsgType } from "../../messages";

export interface ConsoleCommandMessage {
    t: MsgType.ConsoleCommand,
    data: ConsoleCommandMessageData
}

interface ConsoleCommandMessageData {
    commandName: string;
    args: Array<number | string>;
}
