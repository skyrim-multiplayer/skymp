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
const maps: Required<SweetPieMap>[] = [];

// Riften
pointsByName.set('riften:safePlace', {
  pos: [418.4863, -179.2634, 64.0000],
  cellOrWorldDesc: '168df:Skyrim.esm',
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
maps.push({
  safePointName: 'riften:safePlace',
  // spawnPointNames: ['riften:spawnPoint1', 'riften:spawnPoint2', 'riften:spawnPoint3', 'riften:spawnPoint4', 'riften:spawnPoint5', 'riften:spawnPoint6'],
  mainSpawnPointName: 'riften:spawnPoint1',
  safePlaceEnterDoors: ['430a6:Skyrim.esm', '42279:Skyrim.esm'],
  safePlaceLeaveDoors: ['16c3c:Skyrim.esm', '44bd7:Skyrim.esm'],
  leaveRoundDoors: ['42285:Skyrim.esm', '42283:Skyrim.esm']
});

// Whiterun
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
maps.push({
  safePointName: 'whiterun:safePlace',
  // spawnPointNames: ['whiterun:spawnPoint1', 'whiterun:spawnPoint2', 'whiterun:spawnPoint3', 'whiterun:spawnPoint4', 'whiterun:spawnPoint5', 'whiterun:spawnPoint6'],
  mainSpawnPointName: 'whiterun:spawnPoint1',
  safePlaceEnterDoors: ['1a6f4:Skyrim.esm'],
  safePlaceLeaveDoors: ['16072:Skyrim.esm'],
  leaveRoundDoors: ['1b1f3:Skyrim.esm']
});

// Windhelm
pointsByName.set('windhelm:spawnPoint1', {
  pos: [134123.7813, 36661.9023, -12252.2842],
  cellOrWorldDesc: 'd45f0:Skyrim.esm',
  rot: [0, 0, -83.5598],
});
pointsByName.set('windhelm:spawnPoint2', {
  pos: [131426.0625, 41579.3945, -11889.8086],
  cellOrWorldDesc: 'd45f0:Skyrim.esm',
  rot: [0, 0, -143.9488],
});
pointsByName.set('windhelm:spawnPoint3', {
  pos: [132020.8750, 36545.2188, -12260.5020],
  cellOrWorldDesc: 'd45f0:Skyrim.esm',
  rot: [0, 0, -81.8858],
});
pointsByName.set('windhelm:spawnPoint4', {
  pos: [130052.2656, 34946.4414, -11835.0566],
  cellOrWorldDesc: 'd45f0:Skyrim.esm',
  rot: [0, 0, 44.1644],
});
pointsByName.set('windhelm:spawnPoint5', {
  pos: [129352.6328, 40330.7852, -11652.3076],
  cellOrWorldDesc: 'd45f0:Skyrim.esm',
  rot: [0, 0, 130.1079],
});
pointsByName.set('windhelm:spawnPoint6', {
  pos: [131651.7344, 39875.6367, -12101.8936],
  cellOrWorldDesc: 'd45f0:Skyrim.esm',
  rot: [0, 0, -155.4077],
});
maps.push({
  safePointName: 'windhelm:safePlace',
  // spawnPointNames: ['windhelm:spawnPoint1', 'windhelm:spawnPoint2', 'windhelm:spawnPoint3', 'windhelm:spawnPoint4', 'windhelm:spawnPoint5', 'windhelm:spawnPoint6'],
  mainSpawnPointName: 'windhelm:spawnPoint1',
  safePlaceEnterDoors: ['d1882:Skyrim.esm', '42279:Skyrim.esm'],
  safePlaceLeaveDoors: ['d1885:Skyrim.esm', 'd1881:Skyrim.esm'],
  leaveRoundDoors: ['55fca:Skyrim.esm']
});

// Markarth
pointsByName.set('markarth:spawnPoint1', {
  pos: [-174156.5781, 7128.9624, -3105.9287],
  cellOrWorldDesc: '16D71:Skyrim.esm',
  rot: [0, 0, -155.4072],
});
pointsByName.set('markarth:spawnPoint2', {
  pos: [-177508.5000, 1116.4828, -2312.4185],
  cellOrWorldDesc: '16D71:Skyrim.esm',
  rot: [0, 0, 55.6242],
});
pointsByName.set('markarth:spawnPoint3', {
  pos: [-178162.2813, 5273.3086, -2149.9355],
  cellOrWorldDesc: '16D71:Skyrim.esm',
  rot: [0, 0, 55.6245],
});
pointsByName.set('markarth:spawnPoint4', {
  pos: [-175959.4688, 6482.9048, -2727.4172],
  cellOrWorldDesc: '16D71:Skyrim.esm',
  rot: [0, 0, 164.4858],
});
pointsByName.set('markarth:spawnPoint5', {
  pos: [-174378.8750, 4578.2480, -1677.9618],
  cellOrWorldDesc: '16D71:Skyrim.esm',
  rot: [0, 0, -13.1307],
});
pointsByName.set('markarth:spawnPoint6', {
  pos: [-176310.9688, 2410.2986, -3245.0044],
  cellOrWorldDesc: '16D71:Skyrim.esm',
  rot: [0, 0, 78.5424],
});
maps.push({
  safePointName: 'markarth:safePlace',
  //spawnPointNames: ['markarth:spawnPoint1', 'markarth:spawnPoint2', 'markarth:spawnPoint3', 'markarth:spawnPoint4', 'markarth:spawnPoint5', 'markarth:spawnPoint6'],
  mainSpawnPointName: 'markarth:spawnPoint1',
  safePlaceEnterDoors: ['793a4:Skyrim.esm'],
  safePlaceLeaveDoors: ['16e38:Skyrim.esm'],
  leaveRoundDoors: ['1c388:Skyrim.esm']
});

const playerController = MpApiInteractor.makeController(pointsByName);
const gameModeListener = new SweetPieGameModeListener(playerController, maps);
MpApiInteractor.setup(gameModeListener);
