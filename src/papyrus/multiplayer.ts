import { sprintf } from '../../lib/sprintf-js';
import { Mp, PapyrusValue, PapyrusObject } from '../types/mp';
import { getObject, getString, getStringArray } from '../utils/papyrusArgs';

const executeUiCommand = (mp: Mp, self: null, args: PapyrusValue[]): void => {
  const actor = getObject(args, 0);
  const commandType = getString(args, 1);
  const argumentNames = getStringArray(args, 2);
  const tokens = getStringArray(args, 3);
  const alter = getString(args, 4);

  const actorId = mp.getIdFromDesc(actor.desc);

  mp.sendUiMessage(actorId, {
    type: 'COMMAND',
    data: {
      commandType: commandType,
      commandArgs: {
        argumentNames: argumentNames,
        tokens: tokens,
      },
      alter: alter.split('\n'),
    },
  });
};

const log = (mp: Mp, self: null, args: PapyrusValue[]): void => {
  const text = getString(args, 0);
  console.log('[GM]', text);
};

const format = (mp: Mp, self: null, args: PapyrusValue[]): string => {
  const format = getString(args, 0);
  const tokens = getStringArray(args, 1);
  return sprintf(format, ...tokens);
};

const getText = (localization: Localization, mp: Mp, self: null, args: PapyrusValue[]): string => {
  const msgId = getString(args, 0);
  return localization.getText(msgId);
};

const getActorsInStreamZone = (mp: Mp, self: null, args: PapyrusValue[]) => {
  const actor = getObject(args, 0);
  const actorId = mp.getIdFromDesc(actor.desc);

  const res = new Array<PapyrusObject>();
  mp.get(actorId, 'neighbors').forEach((formId) => {
    if (mp.get(formId, 'type') === 'MpActor') {
      res.push({ type: 'form', desc: mp.getDescFromId(formId) });
    }
  });
  return res;
};

const getOnlinePlayers = (mp: Mp): PapyrusObject[] => {
  const res = new Array<PapyrusObject>();
  mp.get(0, 'onlinePlayers').forEach((formId) => {
    res.push({ type: 'form', desc: mp.getDescFromId(formId) });
  });
  return res;
};

const asPerk = (mp: Mp, self: null, args: PapyrusValue[]) => {
  return getObject(args, 0);
};

// TODO: In papyrus 0xff000000 As Int return 0, so i create M.HexToInt
const hexToInt = (mp: Mp, self: null, args: PapyrusValue[]) => {
  return +getString(args, 0);
};

export const localizationDefault: Localization = { getText: (x) => x };

export const register = (mp: Mp, localization: Localization = localizationDefault): void => {
  for (const className of ['Multiplayer', 'M']) {
    mp.registerPapyrusFunction('global', className, 'ExecuteUiCommand', (self, args) =>
      executeUiCommand(mp, self, args)
    );

    mp.registerPapyrusFunction('global', className, 'Log', (self, args) => log(mp, self, args));

    mp.registerPapyrusFunction('global', className, 'Format', (self, args) => format(mp, self, args));

    mp.registerPapyrusFunction('global', className, 'GetText', (self, args) => getText(localization, mp, self, args));

    mp.registerPapyrusFunction('global', className, 'GetActorsInStreamZone', (self, args) =>
      getActorsInStreamZone(mp, self, args)
    );

    mp.registerPapyrusFunction('global', className, 'GetOnlinePlayers', () => getOnlinePlayers(mp));

    mp.registerPapyrusFunction('global', className, 'AsPerk', (self, args) => asPerk(mp, self, args));

    mp.registerPapyrusFunction('global', className, 'HexToInt', (self, args) => hexToInt(mp, self, args));
  }
};

export interface Localization {
  getText(msgId: string): string;
}
