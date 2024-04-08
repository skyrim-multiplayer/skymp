import { MsgType } from "../../messages";

export interface ConsoleCommandMessage { 
    t: MsgType.ConsoleCommand, 
    data: { 
        commandName: string, 
        args: unknown[]
    } 
}
