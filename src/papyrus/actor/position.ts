import { Mp, PapyrusObject, PapyrusValue } from '../../types/mp';
import { getNumber } from '../../utils/papyrusArgs';

export const setPosition = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const [x, y, z] = [getNumber(args, 0), getNumber(args, 1), getNumber(args, 2)];
  mp.set(selfId, 'pos', [x, y, z]);
};
export const getPosition = (mp: Mp, self: PapyrusObject) => mp.get(mp.getIdFromDesc(self.desc), 'pos');
export const getPositionX = (mp: Mp, self: PapyrusObject): number => getPosition(mp, self)[0];
export const getPositionY = (mp: Mp, self: PapyrusObject): number => getPosition(mp, self)[1];
export const getPositionZ = (mp: Mp, self: PapyrusObject): number => getPosition(mp, self)[2];

export const setAngle = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const [x, y, z] = [getNumber(args, 0), getNumber(args, 1), getNumber(args, 2)];
  mp.set(selfId, 'angle', [x, y, z]);
};
export const getAngle = (mp: Mp, self: PapyrusObject) => mp.get(mp.getIdFromDesc(self.desc), 'angle');
export const getAngleX = (mp: Mp, self: PapyrusObject): number => getAngle(mp, self)[0];
export const getAngleY = (mp: Mp, self: PapyrusObject): number => getAngle(mp, self)[1];
export const getAngleZ = (mp: Mp, self: PapyrusObject): number => getAngle(mp, self)[2];
