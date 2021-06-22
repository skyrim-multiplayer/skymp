import { Mp, PapyrusObject, PapyrusValue } from '../../types/mp';
import { getString, getNumber } from '../../utils/papyrusArgs';

export const setActorValue = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const avName = getString(args, 0);
  const avValue = getNumber(args, 1);

  mp.set(selfId, `av${avName}`, avValue);
};

export const getActorValue = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) =>
  mp.get(mp.getIdFromDesc(self.desc), `av${getString(args, 0)}`);

export const damageActorValue = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const avName = getString(args, 0);
  const avValue = getNumber(args, 1);

  mp.set(selfId, `av${avName}Damage`, avValue);
};

export const restoreActorValue = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const avName = getString(args, 0);
  const avValue = getNumber(args, 1);

  mp.set(selfId, `av${avName}Restore`, avValue);
};