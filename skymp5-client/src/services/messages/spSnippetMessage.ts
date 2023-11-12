import { Snippet } from "../../spSnippet";

export type SpSnippetMessage = {
    type: "spSnippet";
} & Snippet;
