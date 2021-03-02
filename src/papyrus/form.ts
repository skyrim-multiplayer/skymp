import { Mp, PapyrusObject } from '../types/mp';

const getFormID = (mp: Mp, self: PapyrusObject) => mp.getIdFromDesc(self.desc);

export const register = (mp: Mp): void => {
  mp.registerPapyrusFunction('method', 'Form', 'GetFormID', (self) => getFormID(mp, self));
};
