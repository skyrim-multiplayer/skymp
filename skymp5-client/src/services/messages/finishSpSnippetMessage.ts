import { MsgType } from "../../messages"

export interface FinishSpSnippetMessage {
    t: MsgType.FinishSpSnippet,
    returnValue: unknown, // TODO: improve type: there should union of possible Papyrus values
    snippetIdx: number,
}
