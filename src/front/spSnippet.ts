import { Game } from "../skyrim-platform/skyrimPlatform";
import * as sp from "../skyrim-platform/skyrimPlatform";

const spAny = sp as Record<string, any>;

export interface Snippet {
  class: string;
  function: string;
  arguments: any[];
  selfId: number;
  snippetIdx: number;
}

const runMethod = async (snippet: Snippet): Promise<any> => {
  const self = Game.getFormEx(snippet.selfId);
  if (!self)
    throw new Error(
      `Unable to find form with id ${snippet.selfId.toString(16)}`
    );
  const selfCasted = spAny[snippet.class].from(self);
  if (!selfCasted)
    throw new Error(
      `Form ${snippet.selfId.toString(16)} is not instance of ${snippet.class}`
    );
  const f = selfCasted[snippet.function];
  return await f.apply(selfCasted, snippet.arguments);
};

const runStatic = async (snippet: Snippet): Promise<any> => {
  const papyrusClass = spAny[snippet.class];
  return await papyrusClass[snippet.function](...snippet.arguments);
};

export const run = (snippet: Snippet): Promise<any> => {
  return snippet.selfId ? runMethod(snippet) : runStatic(snippet);
};
