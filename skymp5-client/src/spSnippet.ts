import { Game, Form } from "skyrimPlatform";
import * as sp from "skyrimPlatform";
import { remoteIdToLocalId } from "./view/worldViewMisc";

const spAny = sp as Record<string, any>;

export interface Snippet {
  class: string;
  function: string;
  arguments: any[];
  selfId: number;
  snippetIdx: number;
}

const deserializeArg = (arg: any) => {
  if (typeof arg === "object") {
    const formId = remoteIdToLocalId(arg.formId);
    const form = Game.getFormEx(formId);
    const gameObject = spAny[arg.type].from(form);
    return gameObject;
  }
  return arg;
};

const runMethod = async (snippet: Snippet): Promise<any> => {
  const selfId = remoteIdToLocalId(snippet.selfId);
  const self = Game.getFormEx(selfId);
  if (!self)
    throw new Error(
      `Unable to find form with id ${selfId.toString(16)}`
    );
  const selfCasted = spAny[snippet.class].from(self);
  if (!selfCasted)
    throw new Error(
      `Form ${selfId.toString(16)} is not instance of ${snippet.class}, form type is ${self.getType()}`
    );
  const f = selfCasted[snippet.function];
  return await f.apply(
    selfCasted,
    snippet.arguments.map((arg) => deserializeArg(arg))
  );
};

const runStatic = async (snippet: Snippet): Promise<any> => {
  const papyrusClass = spAny[snippet.class];
  return await papyrusClass[snippet.function](
    ...snippet.arguments.map((arg) => deserializeArg(arg))
  );
};

export const run = async (snippet: Snippet): Promise<any> => {
  if (snippet.class === "SkympHacks") {
    if (snippet.function === "AddItem" || snippet.function === "RemoveItem") {
      const form = Form.from(deserializeArg(snippet.arguments[0])) as Form;
      const sign = snippet.function === "AddItem" ? "+" : "-";
      const count = snippet.arguments[1];

      let soundId = 0x334ab;
      if (form.getFormID() !== 0xf) soundId = 0x14115;

      (sp.Sound.from(Game.getFormEx(soundId)) as sp.Sound).play(Game.getPlayer());

      if (count > 0)
        sp.Debug.notification(sign + " " + form.getName() + " (" + count + ")");
    } else throw new Error("Unknown SkympHack - " + snippet.function);
    return;
  }
  return snippet.selfId ? runMethod(snippet) : runStatic(snippet);
};
