import { type } from 'os';
import { Ctx } from '../types/ctx';
import { Mp, PapyrusObject, PapyrusValue } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';
import { isArrayEqual } from '../utils/helper';
import { getObject } from '../utils/papyrusArgs';

const getDisplayName = (mp: Mp, self: PapyrusObject): string => {
  const formId = mp.getIdFromDesc(self.desc);

  const appearance = mp.get(formId, 'appearance');
  if (typeof appearance['name'] === 'string') {
    return appearance['name'];
  }
  return '';
};

const getPerkList = (mp: Mp, selfId: number, targetId: number): number[] => {
  return (mp.get(selfId, 'perk') as number[]) ?? [];
};

const setPerkList = (mp: Mp, selfId: number, perkList: number[]): void => {
  mp.set(selfId, 'perk', perkList);
};

const hasPerk = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const perk = getObject(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  const perkId = mp.getIdFromDesc(perk.desc);

  return getPerkList(mp, selfId, perkId).includes(perkId);
};

const addPerk = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const perk = getObject(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  const perkId = mp.getIdFromDesc(perk.desc);

  if (hasPerk(mp, self, args)) return;

  const perkList = getPerkList(mp, selfId, perkId);
  perkList.push(perkId);
  setPerkList(mp, selfId, perkList);
};

const removePerk = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const perk = getObject(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  const perkId = mp.getIdFromDesc(perk.desc);

  if (!hasPerk(mp, self, args)) return;

  const perkList = getPerkList(mp, selfId, perkId);
  perkList.push(perkId);
  setPerkList(
    mp,
    selfId,
    perkList.filter((id: number) => id !== perkId)
  );
};

function activePerksUpdate(ctx: Ctx, isEqual: (arr1: any, arr2: any) => boolean) {
  const ac = ctx.sp.Actor.from(ctx.refr as any);
  if (!ac) return;

  if (!isEqual(ctx.state.lastActivePerks, ctx.value)) {
    const perkIds = ctx.value as number[];

    perkIds.forEach((id) => {
      const newPerk = ctx.sp.Perk.from(ctx.sp.Game.getFormEx(id));
      if (!ac.hasPerk(newPerk)) {
        ac.addPerk(newPerk);
      }
    });

    ctx.state.lastPerks = perkIds;
  }
}
const activePerksUpdateInfo = new FunctionInfo(activePerksUpdate);

// TODO: Convert As Perk don't work, user M.AsPerk in papyrus scripts
export const register = (mp: Mp): void => {
  mp.registerPapyrusFunction('method', 'Actor', 'GetDisplayName', (self) => getDisplayName(mp, self));
  mp.registerPapyrusFunction('method', 'Actor', 'AddPerk', (self, args) => addPerk(mp, self, args));
  mp.registerPapyrusFunction('method', 'Actor', 'RemovePerk', (self, args) => removePerk(mp, self, args));
  mp.registerPapyrusFunction('method', 'Actor', 'HasPerk', (self, args) => hasPerk(mp, self, args));

  mp.makeProperty('perk', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: activePerksUpdateInfo.getText({ isEqual: isArrayEqual }),
    updateNeighbor: '',
  });
};
