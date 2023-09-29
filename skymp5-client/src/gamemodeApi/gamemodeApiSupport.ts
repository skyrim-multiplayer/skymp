import { ObjectReference, on, printConsole, storage } from "skyrimPlatform";
import * as sp from "skyrimPlatform";
import { FormModel } from "../modelSource/model";
import { FormViewArray } from "../view/formViewArray";
import { localIdToRemoteId, remoteIdToLocalId } from "../view/worldViewMisc";

interface GamemodeApiCtx {
  refr: ObjectReference | undefined;
  value: unknown;
  _model: FormModel | undefined;
  sp: typeof sp,
  state: Record<string, unknown> | undefined;
  _view: FormViewArray | undefined;
  i: number;
  getFormIdInServerFormat: (clientsideFormId: number) => number;
  getFormIdInClientFormat: (serversideFormId: number) => number;
  get: (propName: string) => unknown;
  respawn: () => void;
}

export class GamemodeApiSupport {
  static tick() {
    const keys = storage["updateNeighborFunctions_keys"] as Array<string>;
    if (keys && Array.isArray(keys)) {
      this.updateNeighborFunctionsKeys = keys;
    } else {
      this.updateNeighborFunctionsKeys = [];
    }

    // TODO: Shouldn't we check storage value before assignment?
    this.updateNeighborFunctions = storage["updateNeighborFunctions"] as Record<
      string,
      any
    >;
  }

  static setFormViewArray(formViewArray: FormViewArray) {
    this.ctx._view = formViewArray;
  }

  static setI(i: number) {
    this.ctx.i = i;
  }

  static updateNeighbor(refr: ObjectReference, model: FormModel, state: Record<string, unknown>) {
    for (const key of this.updateNeighborFunctionsKeys) {
      const v = (model as Record<string, unknown>)[key];
      // According to docs:
      // In `updateOwner`/`updateNeighbor` equals to a value of a currently processed property.
      // Can't be `undefined` here, since updates are not received for `undefined` property values.
      // In other contexts is always `undefined`.
      if (v !== undefined) {
        this.ctx.refr = refr;
        this.ctx.value = v;
        this.ctx._model = model;
        this.ctx.state = state;
        const f = this.updateNeighborFunctions[key];
        // Actually, 'f' should always be a valid function, but who knows
        try {
          if (f) f(this.ctx);
        } catch (e) {
          printConsole(`'updateNeighbor.${key}' - `, e);
        }
      }
    }
  }

  private static createContext(): GamemodeApiCtx {
    return {
      refr: undefined,
      value: undefined,
      _model: undefined,
      sp: sp,
      state: undefined,
      _view: undefined,
      i: -1,
      getFormIdInServerFormat: (clientsideFormId: number) => {
        return localIdToRemoteId(clientsideFormId);
      },
      getFormIdInClientFormat: (serversideFormId: number) => {
        return remoteIdToLocalId(serversideFormId);
      },
      get(propName: string) {
        return (this._model as Record<string, any>)[propName];
      },
      respawn() {
        this._view!.destroyForm(this.i);
      }
    }
  }

  private static updateNeighborFunctionsKeys = new Array<string>();
  private static updateNeighborFunctions: Record<string, any> = {};
  private static ctx = this.createContext();
}

on("tick", () => GamemodeApiSupport.tick());
