import path from 'path';
import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';

declare const mp: Mp;
declare const ctx: Ctx;
const p: string[][] = [["quitGamePortal", "42f3f:SweetPie.esp", "Quit game"], ["neutralPortal", "42f70:SweetPie.esp", "Whiterun (not started)"], ["redPortal", "42e96:SweetPie.esp", "Coming soon"], ["bluePortal", "42fc1:SweetPie.esp", "Coming soon"]];
export class PortalNamingProperty {
  static init() {
    mp.makeProperty('Renaming', {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(this.clienUpdateNames()).getText(),
      updateNeighbor: ''
    });
    for (let k = 0; k < p.length; k++) {
      mp.set(k, 'Renaming', p[k]);
    }
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

  public static updateDM(players: number, maxPlayers: number) {
    let info = mp.get(1, 'Renaming');
    if (Array.isArray(info)) {
      let portalInfo = info.filter((x) => { typeof x === 'string' });
      if (portalInfo.length === info.length) {
        portalInfo[2] = "Whiterun (" + players + "/" + maxPlayers + ")";
        mp.set(1, 'Renaming', portalInfo);
      }
    }
  }
}
