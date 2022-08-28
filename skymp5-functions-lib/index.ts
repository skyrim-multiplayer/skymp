import { isNull } from 'util';
import { PlayerController } from './src/logic/PlayerController';
import { GameModeListener } from './src/logic/GameModeListener';
import { MpApiInteractor } from './src/mpApiInteractor';
import { BrowserProperty } from './src/props/browserProperty';
import { ChatProperty } from './src/props/chatProperty';
import { CounterProperty } from './src/props/counterProperty';
import { DialogProperty } from './src/props/dialogProperty';
import { DisableCheats } from './src/props/disableCheats';
import { EvalProperty } from './src/props/evalProperty';
import { LocationalData, Mp, PapyrusObject, PapyrusValue } from './src/types/mp';
import { Timer } from './src/utils/timer';

const err = (index: number, x: unknown, expectedTypeName: string): never => {
  throw new TypeError(`The argument with index ${index} has value (${JSON.stringify(x)}) that doesn't meet the requirements of ${expectedTypeName}`);
};

const getArray = (args: PapyrusValue[], index: number, type: string): unknown[] => {
  const x = args[index];
  if (x === null || x === undefined) {
    return [];
  }
  // eslint-disable-next-line valid-typeof
  return Array.isArray(x) && !x.filter((v) => typeof v !== type).length ? (x as unknown[]) : err(index, x, `${type}[]`);
};

export const getObject = (args: PapyrusValue[], index: number): PapyrusObject => {
  const x = args[index];
  return x && typeof x === 'object' && !Array.isArray(x) ? (x as PapyrusObject) : err(index, x, 'PapyrusObject');
};

export const getObjectArray = (args: PapyrusValue[], index: number): PapyrusObject[] => {
  return getArray(args, index, 'object') as PapyrusObject[];
};

export const getString = (args: PapyrusValue[], index: number): string => {
  const x = args[index];
  return typeof x === 'string' ? x : err(index, x, 'string');
};

export const getStringArray = (args: PapyrusValue[], index: number): string[] => {
  return getArray(args, index, 'string') as string[];
};

export const getNumber = (args: PapyrusValue[], index: number): number => {
  const x = args[index];
  return typeof x === 'number' ? x : err(index, x, 'number');
};

export const getNumberArray = (args: PapyrusValue[], index: number): number[] => {
  return getArray(args, index, 'number') as number[];
};

export const getBoolean = (args: PapyrusValue[], index: number): boolean => {
  const x = args[index];
  return typeof x === 'boolean' ? x : err(index, x, 'boolean');
};

export const getBooleanArray = (args: PapyrusValue[], index: number): boolean[] => {
  return getArray(args, index, 'boolean') as boolean[];
};

export const randomInt = (mp: Mp, self: null, args: PapyrusValue[]): number => {
  const min = getNumber(args, 0);
  const max = getNumber(args, 1);
  const randomInRangeBothInclusive = (min: number, max: number) => Math.floor(Math.random() * (max - min + 1)) + min;
  return randomInRangeBothInclusive(min, max);
};

// Game.getForm
export const getForm = (mp: Mp, self: null, args: PapyrusValue[]): PapyrusObject | undefined => {
  const formId = getNumber(args, 0);
  try {
    if (formId >= 0xff000000) {
      mp.get(formId, 'type');
      return {
        desc: mp.getDescFromId(formId),
        type: 'form',
      };
    }
    const espm = mp.lookupEspmRecordById(formId);
    if (!espm.record?.type) {
      console.log(`ESPM Record by id ${formId.toString(16)} not found`);
      return;
    }
    const obj: PapyrusObject = {
      desc: mp.getDescFromId(formId),
      type: espm.record.type,
    };
    return obj;
  } catch (err) {
    const regex = /Form with id.+doesn't exist/gm;
    if (regex.exec(err as string) !== null) {
      console.log(err);
      return;
    }
    console.log(err);
    throw err;
  }
};

export const moveTo = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]): PapyrusObject | undefined => {
  const akTarget = getObject(args, 0);
  const afXOffset = getNumber(args, 1);
  const afYOffset = getNumber(args, 2);
  const afZOffset = getNumber(args, 3);
  const abMatchRotation = getBoolean(args, 4);

  if (self.type !== 'form') {
    console.log("moveTo - Can't teleport espms");
    return undefined;
  }

  const selfId = mp.getIdFromDesc(self.desc);
  if (selfId < 0xff000000) {
    console.log("moveTo - Can't teleport forms with id < 0xff000000 (TODO)")
    return undefined;
  }

  const targetId = mp.getIdFromDesc(akTarget.desc);

  const targetPos = mp.get(targetId, "pos");
  const targetAngle = mp.get(targetId, "angle");
  const targetCellOrWorldDesc = mp.get(targetId, "worldOrCellDesc");
  const selfAngle = mp.get(selfId, "angle");

  targetPos[0] += afXOffset;
  targetPos[1] += afYOffset;
  targetPos[2] += afZOffset;

  const newLocationalData: LocationalData = {
    cellOrWorldDesc: targetCellOrWorldDesc,
    pos: targetPos,
    rot: abMatchRotation ? targetAngle : selfAngle
  };
  mp.set(selfId, "locationalData", newLocationalData);

  return undefined;
}

export const isDead = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]): boolean => {
  const selfId = mp.getIdFromDesc(self.desc);
  return mp.get(selfId, 'isDead');
}

const printArgs = (args: PapyrusValue[]): string => {
  var out = '';
  for (const pVal of args) {
    out += ' ' + ((typeof pVal === 'object' || Array.isArray(pVal)) ? JSON.stringify(pVal) : pVal) + ';';
  }
  return out;
}

export const placeholder = (mp: Mp, self: PapyrusObject | null, args: PapyrusValue[], name: string): PapyrusObject | undefined => {
  var str = name + (self ? ':' + self.desc + ':' : ':') + printArgs(args);
  console.log(str);
  return undefined;
}

// TODO: Move these types to a better place
type SPExchangeRet = {
  formId: number, returnNumber: number,
}

type SPExchangeReq = {
  formId: number, requiredCount: number,
  returnSelect: 'random' | 'none', returns: SPExchangeRet[],
}

type SPExchange = {
  formId: number, commissionItem: number,
  commissionSize: number, startMessage: number,
  failMessage: number, finishMessage: number,
  reqs: SPExchangeReq[],
}

// TODO: Get these values from external config file
const spExchanges: SPExchange[] = [
  {
    formId: 0x084d2b03,
    commissionItem: 0x0f,
    commissionSize: 1,
    startMessage: 0x0835d1f0,
    failMessage: 0x0835d1f2,
    finishMessage: 0x0835d1f1,
    reqs: [
      {
        formId: 0x0f,
        requiredCount: 30,
        returnSelect: 'random',
        returns: [
          {
            formId: 0x0300353a,
            returnNumber: 1,
          },
          {
            formId: 0x03003539,
            returnNumber: 1,
          },
          {
            formId: 0x0300353b,
            returnNumber: 1,
          },
          {
            formId: 0x00064b43,
            returnNumber: 1,
          },
        ],
      },
    ],
  },
  {
    formId: 0x0851f7e4,
    commissionItem: 0x0f,
    commissionSize: 1,
    startMessage: 0x085390f9,
    failMessage: 0x085390fa,
    finishMessage: 0x085390fb,
    reqs: [
      {
        formId: 0x05ad9e,
        requiredCount: 1,
        returnSelect: 'none',
        returns: [
          {
            formId: 0x0f,
            returnNumber: 30,
          },
        ],
      },
      {
        formId: 0x05ace3,
        requiredCount: 1,
        returnSelect: 'none',
        returns: [
          {
            formId: 0x0f,
            returnNumber: 20,
          },
        ],
      },
    ],
  },
];

export const getLicenses = (mp: Mp, self: null, args: PapyrusValue[], exchanges: SPExchange[]): number[] => {
  const ret: number[] = [];
  for (const l of exchanges) {
    ret.push(l.formId);
  }
  return ret;
}

export const getRequiredItems = (mp: Mp, self: null, args: PapyrusValue[], exchanges: SPExchange[]): number[] => {
  const ret: number[] = [];
  const licenseIndex: number = getNumber(args, 0);
  if (exchanges[licenseIndex].reqs.length) {
    for (const l of exchanges[licenseIndex].reqs) {
      ret.push(l.formId);
    }
  }
  return ret;
}

export const getSPExchangeNumberField = (mp: Mp, self: null, args: PapyrusValue[], exchanges: SPExchange[],
  field: 'startMessage' | 'failMessage' | 'finishMessage' | 'commissionItem' | 'commissionSize'): number => {
  const ret: number = 0;
  const licenseIndex: number = getNumber(args, 0);
  if (exchanges[licenseIndex]) {
    return exchanges[licenseIndex][field];
  }
  return ret;
}

export const getRequiredItemCount = (mp: Mp, self: null, args: PapyrusValue[], exchanges: SPExchange[]): number => {
  const ret: number = 0;
  const licenseIndex: number = getNumber(args, 0);
  const requiredItemIndex: number = getNumber(args, 1);
  if (exchanges[licenseIndex].reqs[requiredItemIndex]) {
    return exchanges[licenseIndex].reqs[requiredItemIndex].requiredCount;
  }
  return ret;
}

export const getReturnItemIndex = (mp: Mp, self: null, args: PapyrusValue[], exchanges: SPExchange[]): number => {
  const ret: number = 0;
  const licenseIndex: number = getNumber(args, 0);
  const requiredItemIndex: number = getNumber(args, 1);
  if (exchanges[licenseIndex].reqs[requiredItemIndex].returns.length) {
    if (exchanges[licenseIndex].reqs[requiredItemIndex].returnSelect === 'random') {
      const max = exchanges[licenseIndex].reqs[requiredItemIndex].returns.length - 1;
      return randomInt(mp, self, [0,max]);
    }
  }
  return ret;
}

export const getReturnItem = (mp: Mp, self: null, args: PapyrusValue[], exchanges: SPExchange[]): number => {
  const ret: number = 0;
  const licenseIndex: number = getNumber(args, 0);
  const requiredItemIndex: number = getNumber(args, 1);
  const returnItemIndex: number = getNumber(args, 2);
  if (exchanges[licenseIndex].reqs[requiredItemIndex].returns[returnItemIndex]) {
    return exchanges[licenseIndex].reqs[requiredItemIndex].returns[returnItemIndex].formId;
  }
  return ret;
}

export const getReturnItemCount = (mp: Mp, self: null, args: PapyrusValue[], exchanges: SPExchange[]): number => {
  const ret: number = 0;
  const licenseIndex: number = getNumber(args, 0);
  const requiredItemIndex: number = getNumber(args, 1);
  const returnItemIndex: number = getNumber(args, 2);
  if (exchanges[licenseIndex].reqs[requiredItemIndex].returns[returnItemIndex]) {
    return exchanges[licenseIndex].reqs[requiredItemIndex].returns[returnItemIndex].returnNumber;
  }
  return ret;
}

DialogProperty.init();
BrowserProperty.init();
EvalProperty.init();
ChatProperty.init();
CounterProperty.init();
Timer.init();
DisableCheats.init();

declare const mp: Mp;
mp.registerPapyrusFunction('global', 'Utility', 'RandomInt', (self, args) => randomInt(mp, self, args));
mp.registerPapyrusFunction('global', 'Game', 'GetForm', (self, args) => getForm(mp, self, args));
mp.registerPapyrusFunction('global', 'Game', 'GetFormEx', (self, args) => getForm(mp, self, args));
mp.registerPapyrusFunction('method', 'ObjectReference', 'MoveTo', (self, args) => moveTo(mp, self, args));
mp.registerPapyrusFunction('method', 'Actor', 'IsDead', (self, args) => isDead(mp, self, args));
console.log('gamemode.js reloaded');

const pointsByName = new Map<string, LocationalData>();
pointsByName.set('riverwood:spawnPoint', {
  pos: [19256.69, 9777.12, 899.56],
  cellOrWorldDesc: '133C6:Skyrim.esm',
  rot: [6.36, 0.00, 221.51],
});

const createGameModeListener = (controller: PlayerController): GameModeListener => {
    return new GameModeListener(controller);
};

const playerController = MpApiInteractor.makeController(pointsByName);
const gameModeListener = createGameModeListener(playerController);
MpApiInteractor.setup(gameModeListener);
