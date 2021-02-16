import { Mp, PapyrusValue } from '../types/mp';
import { getNumber, getString } from '../utils/papyrusArgs';

const createStringArray = (mp: Mp, self: null, args: PapyrusValue[]): string[] => {
  const size = getNumber(args, 0);
  const fill = getString(args, 1);
  return new Array<string>(size).fill(fill);
};

export const register = (mp: Mp): void => {
  mp.registerPapyrusFunction('global', 'Utility', 'CreateStringArray', (self, args) =>
    createStringArray(mp, self, args)
  );
};
