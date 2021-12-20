import { Mp, PapyrusValue, PapyrusObject } from '../types/mp';
import { getNumber } from '../utils/papyrusArgs';

const getForm = (mp: Mp, self: null, args: PapyrusValue[]): PapyrusObject | undefined => {
  const formId = getNumber(args, 0);

  try {
    if (formId >= 0xff000000) {
      mp.get(formId, 'type');
      return {
        desc: mp.getDescFromId(formId),
        type: 'form',
      };
    } else {
      const espm = mp.lookupEspmRecordById(formId);
      if (!espm.record?.type) return;

      return {
        desc: mp.getDescFromId(formId),
        type: ['REFR', 'ACHR'].includes(espm.record?.type) ? 'form' : 'espm',
      };
    }
  } catch (err) {
    const regex = /Form with id.+doesn't exist/gm;
    if (regex.exec(err) !== null) {
      return;
    }
    throw err;
  }
};

export const register = (mp: Mp): void => {
  mp.registerPapyrusFunction('global', 'Game', 'GetForm', (self, args) => getForm(mp, self, args));
  mp.registerPapyrusFunction('global', 'Game', 'GetFormEx', (self, args) => getForm(mp, self, args));
};
