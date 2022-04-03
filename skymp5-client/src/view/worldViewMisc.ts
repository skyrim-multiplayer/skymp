import { storage } from "skyrimPlatform";
import { WorldView } from "./worldView";

export const getViewFromStorage = (): WorldView | undefined => {
  const res = storage.view as WorldView;
  if (typeof res === "object") return res;
  return undefined;
};

export const localIdToRemoteId = (localFormId: number): number => {
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
