export type SpSnippetMessage = {
    type: "spSnippet";
    class: string;
    function: string;
    arguments: any[];
    selfId: number;
    snippetIdx: number;
};
