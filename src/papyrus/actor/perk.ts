import { Mp, PapyrusObject, PapyrusValue } from '../../types/mp';
import { getObject } from '../../utils/papyrusArgs';

const getPerkList = (mp: Mp, selfId: number): number[] => {
  return (mp.get(selfId, 'perk') as number[]) ?? [];
};

const setPerkList = (mp: Mp, selfId: number, perkList: number[]): void => {
  mp.set(selfId, 'perk', perkList);
};

export const hasPerk = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const perk = getObject(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  const perkId = mp.getIdFromDesc(perk.desc);

  return getPerkList(mp, selfId).includes(perkId);
};

export const addPerk = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const perk = getObject(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  const perkId = mp.getIdFromDesc(perk.desc);

  if (hasPerk(mp, self, args)) return;

  const perkList = getPerkList(mp, selfId);
  perkList.push(perkId);
  setPerkList(mp, selfId, perkList);
};

export const removePerk = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const perk = getObject(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  const perkId = mp.getIdFromDesc(perk.desc);

  if (!hasPerk(mp, self, args)) return;

  const perkList = getPerkList(mp, selfId);
  perkList.push(perkId);
  setPerkList(
    mp,
    selfId,
    perkList.filter((id: number) => id !== perkId)
  );
};
