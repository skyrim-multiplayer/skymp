import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';

declare const mp: Mp;
declare const ctx: Ctx;

export class PortalNamingProperty {
  static init() {
    mp.makeProperty('Renaming', {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(this.clienUpdateNames()).getText(),
      updateNeighbor: ''
    });
  }

  public static clienUpdateNames() {
    return () => {
      const portals = mp.get(0, 'Renaming');
      const names = mp.get(1, 'Renaming');
      if (Array.isArray(portals) && Array.isArray(names)) {
        const len = portals.length;
        if (len === names.length) {
          for (let k = 0; k < len; k++) {
            if (typeof portals[k] === "string" && typeof names[k] === "string") {
              ctx.sp.setTextString(mp.getIdFromDesc(portals[k]), names[k]);
            }
          }
        }
      }
    };
  }

  public static set(names: string[]) {
    mp.set(1, 'Renaming', names);
  }
}
