import { Mp, PapyrusValue } from '../types/mp';
import { getString, getNumber } from '../utils/papyrusArgs';

const getNthChar = (mp: Mp, self: null, args: PapyrusValue[]) => {
  const text = getString(args, 0);
  const index = getNumber(args, 1);
  return text[index];
};

const split = (mp: Mp, self: null, args: PapyrusValue[]) => {
  const text = getString(args, 0);
  const splitter = getString(args, 1);
  return text.split(splitter);
};

const substring = (mp: Mp, self: null, args: PapyrusValue[]) => {
  const s = getString(args, 0);
  const startIndex = getNumber(args, 1);
  const length = getNumber(args, 2);
  return s.substring(startIndex, length ? startIndex + length : undefined);
};

const getLength = (mp: Mp, self: null, args: PapyrusValue[]) => {
  const s = getString(args, 0);
  return s.length;
};

const toLower = (mp: Mp, self: null, args: PapyrusValue[]) => {
  const s = getString(args, 0);
  return s.toLowerCase();
};

export const register = (mp: Mp): void => {
  mp.registerPapyrusFunction('global', 'StringUtil', 'GetNthChar', (self, args) => getNthChar(mp, self, args));
  mp.registerPapyrusFunction('global', 'StringUtil', 'Split', (self, args) => split(mp, self, args));
  mp.registerPapyrusFunction('global', 'StringUtil', 'Substring', (self, args) => substring(mp, self, args));
  mp.registerPapyrusFunction('global', 'StringUtil', 'GetLength', (self, args) => getLength(mp, self, args));
  mp.registerPapyrusFunction('global', 'StringUtil', 'ToLower', (self, args) => toLower(mp, self, args));
};
