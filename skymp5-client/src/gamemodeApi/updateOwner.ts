import * as sp from "skyrimPlatform";
import { FormModel } from "../modelSource/model";
import { localIdToRemoteId, remoteIdToLocalId } from "../view/worldViewMisc";

export const setOwnerModel = (ownerModel: FormModel): void => {
  sp.storage["ownerModel"] = ownerModel;
  sp.storage["ownerModelSet"] = true;
};

export const setup = (): void => {
  const ctx = {
    sp: sp,
    refr: undefined as unknown as sp.ObjectReference,
    value: undefined as unknown,
    _model: undefined as unknown as FormModel,
    getFormIdInServerFormat: (clientsideFormId: number) => {
      return localIdToRemoteId(clientsideFormId);
    },
    getFormIdInClientFormat: (serversideFormId: number) => {
      return remoteIdToLocalId(serversideFormId);
    },
    get(propName: string) {
      return (this._model as Record<string, any>)[propName];
    },
    state: {},
  };
  sp.on("update", () => {
    let keys = sp.storage["updateOwnerFunctions_keys"] as Array<string>;
    if (!keys || !Array.isArray(keys)) {
      keys = [];
    }
    const funcs = sp.storage["updateOwnerFunctions"];

    if (sp.storage["ownerModelSet"] !== true) return;

    const ownerModel = sp.storage["ownerModel"] as FormModel;

    for (const propName of keys) {
      const f = (funcs as Record<string, any>)[propName];
      // Actually, must always be a valid funciton, but who knows
      if (!f) continue;

      ctx._model = ownerModel;
      if (!ctx._model) continue;

      ctx.value = (ctx._model as Record<string, unknown>)[propName];
      if (ctx.value === undefined) continue;

      ctx.refr = sp.ObjectReference.from(
        sp.Game.getPlayer()
      ) as sp.ObjectReference;
      ctx._model = ownerModel;
      try {
        if (f) f(ctx);
      } catch (e) {
        sp.printConsole(`'updateOwner.${propName}' - `, e);
      }
    }
  });
};
