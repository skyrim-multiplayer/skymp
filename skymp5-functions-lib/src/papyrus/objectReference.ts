import { Mp, PapyrusValue, PapyrusObject } from '../types/mp';
import { getNumber, getObject } from '../utils/papyrusArgs';

const getDistance = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]): number => {
  const target = getObject(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  const targetId = mp.getIdFromDesc(target.desc);

  const selfPos = mp.get(selfId, 'pos');
  const targetPos = mp.get(targetId, 'pos');

  return Math.sqrt(
    Math.pow(selfPos[0] - targetPos[0], 2) +
      Math.pow(selfPos[1] - targetPos[1], 2) +
      Math.pow(selfPos[2] - targetPos[2], 2)
  );
};

const setScale = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]): void => {
  const scale = getNumber(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  // ...
};

export const register = (mp: Mp): void => {
  mp.registerPapyrusFunction('method', 'ObjectReference', 'GetDistance', (self, args) => getDistance(mp, self, args));
  mp.registerPapyrusFunction('method', 'ObjectReference', 'SetScale', (self, args) => setScale(mp, self, args));
};
