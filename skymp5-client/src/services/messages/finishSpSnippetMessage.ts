import { MsgType } from "../../messages"

export interface FinishSpSnippetMessage {
    t: MsgType.FinishSpSnippet,
    returnValue?: boolean | number | string;
    snippetIdx: number,
}
