import { Game, ObjectReference, storage } from "skyrimPlatform";
import { WorldView } from "./worldView";
import { SpApiInteractor } from '../services/spApiInteractor';
import { RemoteServer } from "../services/services/remoteServer";

export const getViewFromStorage = (): WorldView | undefined => {
  const res = storage["view"] as WorldView;
  // can't use instanceof here because each hot reload creates a new class
  if (typeof res === "object") return res;
  return undefined;
};

export const localIdToRemoteId = (localFormId: number, newCast: boolean = false): number => {
  if (newCast && localFormId == 0x14) {
    return SpApiInteractor.getControllerInstance().lookupListener(RemoteServer).getMyRemoteRefrId();
  }

  if (localFormId >= 0xff000000) {
    const view = getViewFromStorage();
    if (!view) return 0;
    localFormId = view.getRemoteRefrId(localFormId);
    if (!localFormId) return 0;
    // serverside ids are 64bit
    if (localFormId >= 0x100000000) {
      localFormId -= 0x100000000;
    }
  }
  return localFormId;
};

export const remoteIdToLocalId = (remoteFormId: number): number => {
  if (remoteFormId >= 0xff000000) {
    const view = getViewFromStorage();
    if (!view) return 0;
    remoteFormId = view.getLocalRefrId(remoteFormId);
    if (!remoteFormId) return 0;
  }
  return remoteFormId;
};

export const getObjectReference = (i: number): ObjectReference | null => {
  const view = getViewFromStorage();
  if (view) {
    const formView = view.getFormViews().getNthFormView(i);
    if (formView) {
      const refrId = formView.getLocalRefrId();
      if (refrId > 0) {
        const refr = ObjectReference.from(Game.getFormEx(refrId));
        if (refr !== null) {
          return refr;
        }
      }
    }
  }
  return null;
};
