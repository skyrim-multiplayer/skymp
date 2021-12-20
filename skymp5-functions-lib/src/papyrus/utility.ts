import { Mp, PapyrusValue } from '../types/mp';
import { randomInRange } from '../utils/helper';
import { getNumber, getString } from '../utils/papyrusArgs';

const createStringArray = (mp: Mp, self: null, args: PapyrusValue[]): string[] => {
  const size = getNumber(args, 0);
  const fill = getString(args, 1);
  return new Array<string>(size).fill(fill);
};

const randomInt = (mp: Mp, self: null, args: PapyrusValue[]): number => {
  const min = getNumber(args, 0);
  const max = getNumber(args, 1);
  return randomInRange(min, max);
};

export const register = (mp: Mp): void => {
  mp.registerPapyrusFunction('global', 'Utility', 'CreateStringArray', (self, args) =>
    createStringArray(mp, self, args)
  );
  mp.registerPapyrusFunction('global', 'Utility', 'RandomInt', (self, args) => randomInt(mp, self, args));
};
