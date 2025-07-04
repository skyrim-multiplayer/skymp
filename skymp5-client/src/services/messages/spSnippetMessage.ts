import { MsgType } from "../../messages";

export interface SpSnippetMessage {
    t: MsgType.SpSnippet;
    class: string;
    function: string;
    arguments: any[];
    selfId: number;
    snippetIdx: number;
};
