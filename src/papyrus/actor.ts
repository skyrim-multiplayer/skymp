import { Mp, PapyrusObject } from '../types/mp';

const getDisplayName = (mp: Mp, self: PapyrusObject): string => {
  const formId = mp.getIdFromDesc(self.desc);

  const appearance = mp.get(formId, 'appearance');
  if (typeof appearance['name'] === 'string') {
    return appearance['name'];
  }
  return '';
};

export const register = (mp: Mp): void => {
  mp.registerPapyrusFunction('method', 'Actor', 'GetDisplayName', (self) => getDisplayName(mp, self));
};
