import { PlayerController } from './src/logic/PlayerController';
import { SweetPieGameModeListener } from './src/logic/SweetPieGameModeListener';
import { SweetPieMap } from './src/logic/SweetPieMap';
import { MpApiInteractor } from './src/mpApiInteractor';
import { BrowserProperty } from './src/props/browserProperty';
import { ChatProperty } from './src/props/chatProperty';
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
      type: ['REFR', 'ACHR'].includes(espm.record?.type) ? 'form' : 'espm',
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

DialogProperty.init();
BrowserProperty.init();
EvalProperty.init();
ChatProperty.init();
Timer.init();
DisableCheats.init();

declare const mp: Mp;
mp.registerPapyrusFunction('global', 'Utility', 'RandomInt', (self, args) => randomInt(mp, self, args));
mp.registerPapyrusFunction('global', 'Game', 'GetForm', (self, args) => getForm(mp, self, args));
mp.registerPapyrusFunction('global', 'Game', 'GetFormEx', (self, args) => getForm(mp, self, args));
mp.registerPapyrusFunction('method', 'ObjectReference', 'MoveTo', (self, args) => moveTo(mp, self, args));

console.log('gamemode.js reloaded');

const pointsByName = new Map<string, LocationalData>();
pointsByName.set('hall:spawnPoint', {
  pos: [18522.08, 10218.17, 624.46],
  cellOrWorldDesc: '42b5f:SweetPie.esp',
  rot: [0, 0, 0],
});
pointsByName.set('riften:safePlace', {
  pos: [418.4863, -179.2634, 64.0000],
  cellOrWorldDesc: '16bdf:Skyrim.esm',
  rot: [0, 0, -88.6934],
});
pointsByName.set('riften:spawnPoint1', {
  pos: [172414.4688, -99692.1719, 11136.5918],
  cellOrWorldDesc: '16bb4:Skyrim.esm',
  rot: [0, 0, 177.6169],
});
pointsByName.set('riften:spawnPoint2', {
  pos: [174379.4063, -93622.4688, 11125.2783],
  cellOrWorldDesc: '16bb4:Skyrim.esm',
  rot: [0, 0, 4.0566],
});
pointsByName.set('riften:spawnPoint3', {
  pos: [172683.5625, -93227.9766, 11221.7686],
  cellOrWorldDesc: '16bb4:Skyrim.esm',
  rot: [0, 0, 177.6169],
});
pointsByName.set('riften:spawnPoint4', {
  pos: [174862.0938, -95209.4141, 11397.0518],
  cellOrWorldDesc: '16bb4:Skyrim.esm',
  rot: [0, 0, 269.2902],
});
pointsByName.set('riften:spawnPoint5', {
  pos: [175102.6563, -97816.5859, 11139.4395],
  cellOrWorldDesc: '16bb4:Skyrim.esm',
  rot: [0, 0, -70.4278],
});
pointsByName.set('riften:spawnPoint6', {
  pos: [171816.8125, -96627.7891, 11136.0000],
  cellOrWorldDesc: '16bb4:Skyrim.esm',
  rot: [0, 0, 111.9557],
});
pointsByName.set('whiterun:spawnPoint', {
  pos: [22659, -8697, -3594],
  cellOrWorldDesc: '1a26f:Skyrim.esm',
  rot: [0, 0, 268],
});
pointsByName.set('whiterun:safePlace', {
  pos: [-108, -809, 69.25],
  cellOrWorldDesc: '1605e:Skyrim.esm',
  rot: [0, 0, 176],
});
pointsByName.set('whiterun:spawnPoint1', {
  pos: [26496.7656, -6511.5684, -3183.8271],
  cellOrWorldDesc: '1a26f:Skyrim.esm',
  rot: [0, 0, -81.8869],
});
pointsByName.set('whiterun:spawnPoint2', {
  pos: [19170.6484, -6968.4121, -3549.6443],
  cellOrWorldDesc: '1a26f:Skyrim.esm',
  rot: [0, 0, 72.8116],
});
pointsByName.set('whiterun:spawnPoint3', {
  pos: [20930.9316, -9920.6191, -3497.4736],
  cellOrWorldDesc: '1a26f:Skyrim.esm',
  rot: [0, 0, 32.7047],
});
pointsByName.set('whiterun:spawnPoint4', {
  pos: [20287.3750, -6478.4185, -3225.1785],
  cellOrWorldDesc: '1a26f:Skyrim.esm',
  rot: [0, 0, 170.2144],
});
pointsByName.set('whiterun:spawnPoint5', {
  pos: [24443.8594, -11096.1328, -3292.7659],
  cellOrWorldDesc: '1a26f:Skyrim.esm',
  rot: [0, 0, -1.6730],
});
pointsByName.set('whiterun:spawnPoint6', {
  pos: [22855.9473, -6104.4355, -3092.7661],
  cellOrWorldDesc: '1a26f:Skyrim.esm',
  rot: [0, 0, 84.2714],
});

const maps: Required<SweetPieMap>[] = [{
  safePointName: 'riften:safePlace',
  mainSpawnPointName: 'riften:spawnPoint1',
  safePlaceEnterDoors: ['430a6:Skyrim.esm', '42279:Skyrim.esm'],
  safePlaceLeaveDoors: ['16c3c:Skyrim.esm', '44bd7:Skyrim.esm'],
  leaveRoundDoors: ['42285:Skyrim.esm', '42283:Skyrim.esm'],
  playerRestoreActivators: ['2b46a7:SweetPie.esp'],
  playerRestoreWaitTime: 30000,
  spawnPointNames: ['riften:spawnPoint1', 'riften:spawnPoint2', 'riften:spawnPoint3', 'riften:spawnPoint4', 'riften:spawnPoint5', 'riften:spawnPoint6'],
},
{
  safePointName: 'whiterun:safePlace',
  mainSpawnPointName: 'whiterun:spawnPoint',
  safePlaceEnterDoors: ['1a6f4:Skyrim.esm'],
  safePlaceLeaveDoors: ['16072:Skyrim.esm'],
  leaveRoundDoors: ['1b1f3:Skyrim.esm'],
  playerRestoreActivators: ['3a99d6:SweetPie.esp'],
  playerRestoreWaitTime: 30000,
  spawnPointNames: ['whiterun:spawnPoint1', 'whiterun:spawnPoint2', 'whiterun:spawnPoint3', 'whiterun:spawnPoint4', 'whiterun:spawnPoint5', 'whiterun:spawnPoint6'],
}];

const createGameModeListener = (controller: PlayerController, maps: SweetPieMap[], playersToStart: unknown): SweetPieGameModeListener => {
  if (typeof playersToStart === "number") {
    return new SweetPieGameModeListener(controller, maps, playersToStart);
  } else {
    return new SweetPieGameModeListener(controller, maps);
  }
};

const playerController = MpApiInteractor.makeController(pointsByName);
const gameModeListener = createGameModeListener(playerController, maps, mp.getServerSettings()["sweetPieMinimumPlayersToStart"]);
MpApiInteractor.setup(gameModeListener);
