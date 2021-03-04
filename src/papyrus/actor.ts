import { evalClient } from '../properties/eval';
import { Ctx } from '../types/ctx';
import { Mp, PapyrusObject, PapyrusValue } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';
import { getBoolean, getNumber, getObject } from '../utils/papyrusArgs';

const getDisplayName = (mp: Mp, self: PapyrusObject): string => {
  const formId = mp.getIdFromDesc(self.desc);

  const appearance = mp.get(formId, 'appearance');
  if (typeof appearance['name'] === 'string') {
    return appearance['name'];
  }
  return '';
};

const getPerkList = (mp: Mp, selfId: number): number[] => {
  return (mp.get(selfId, 'perk') as number[]) ?? [];
};

const setPerkList = (mp: Mp, selfId: number, perkList: number[]): void => {
  mp.set(selfId, 'perk', perkList);
};

const hasPerk = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const perk = getObject(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  const perkId = mp.getIdFromDesc(perk.desc);

  return getPerkList(mp, selfId).includes(perkId);
};

const addPerk = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const perk = getObject(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  const perkId = mp.getIdFromDesc(perk.desc);

  if (hasPerk(mp, self, args)) return;

  const perkList = getPerkList(mp, selfId);
  perkList.push(perkId);
  setPerkList(mp, selfId, perkList);
};

const removePerk = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const perk = getObject(args, 0);
  const selfId = mp.getIdFromDesc(self.desc);
  const perkId = mp.getIdFromDesc(perk.desc);

  if (!hasPerk(mp, self, args)) return;

  const perkList = getPerkList(mp, selfId);
  perkList.push(perkId);
  setPerkList(
    mp,
    selfId,
    perkList.filter((id: number) => id !== perkId)
  );
};

// TODO: add item if not exists?
const equipItem = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const item = getObject(args, 0);
  const itemId = mp.getIdFromDesc(item.desc);
  const preventRemoval = args[1] ? getBoolean(args, 1) : false;
  const silent = args[2] ? getBoolean(args, 2) : false;

  const equipFunction = (ctx: Ctx, itemId: number) => {
    ctx.sp.once('update', () => {
      const form = ctx.sp.Game.getFormEx(itemId);
      ctx.sp.Game.getPlayer()?.equipItem(form, false, true);
    });
  };
  evalClient(mp, selfId, new FunctionInfo(equipFunction).getText({ itemId }));

  if (!silent) {
    // TODO: debug.notification
  }
};

const setPosition = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const [x, y, z] = [getNumber(args, 0), getNumber(args, 1), getNumber(args, 2)];
  mp.set(selfId, 'pos', [x, y, z]);
};
const getPosition = (mp: Mp, self: PapyrusObject) => mp.get(mp.getIdFromDesc(self.desc), 'pos');
const getPositionX = (mp: Mp, self: PapyrusObject): number => getPosition(mp, self)[0];
const getPositionY = (mp: Mp, self: PapyrusObject): number => getPosition(mp, self)[1];
const getPositionZ = (mp: Mp, self: PapyrusObject): number => getPosition(mp, self)[2];

const setAngle = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const [x, y, z] = [getNumber(args, 0), getNumber(args, 1), getNumber(args, 2)];
  mp.set(selfId, 'angle', [x, y, z]);
};
const getAngle = (mp: Mp, self: PapyrusObject) => mp.get(mp.getIdFromDesc(self.desc), 'angle');
const getAngleX = (mp: Mp, self: PapyrusObject): number => getAngle(mp, self)[0];
const getAngleY = (mp: Mp, self: PapyrusObject): number => getAngle(mp, self)[1];
const getAngleZ = (mp: Mp, self: PapyrusObject): number => getAngle(mp, self)[2];

// TODO: Convert As Perk don't work, user M.AsPerk in papyrus scripts
export const register = (mp: Mp): void => {
  mp.registerPapyrusFunction('method', 'Actor', 'GetDisplayName', (self) => getDisplayName(mp, self));

  mp.registerPapyrusFunction('method', 'Actor', 'AddPerk', (self, args) => addPerk(mp, self, args));
  mp.registerPapyrusFunction('method', 'Actor', 'RemovePerk', (self, args) => removePerk(mp, self, args));
  mp.registerPapyrusFunction('method', 'Actor', 'HasPerk', (self, args) => hasPerk(mp, self, args));

  mp.registerPapyrusFunction('method', 'Actor', 'EquipItem', (self, args) => equipItem(mp, self, args));

  mp.registerPapyrusFunction('method', 'Actor', 'SetPosition', (self, args) => setPosition(mp, self, args));
  mp.registerPapyrusFunction('method', 'Actor', 'GetPositionX', (self) => getPositionX(mp, self));
  mp.registerPapyrusFunction('method', 'Actor', 'GetPositionY', (self) => getPositionY(mp, self));
  mp.registerPapyrusFunction('method', 'Actor', 'GetPositionZ', (self) => getPositionZ(mp, self));

  mp.registerPapyrusFunction('method', 'Actor', 'SetAngle', (self, args) => setAngle(mp, self, args));
  mp.registerPapyrusFunction('method', 'Actor', 'GetAngleX', (self) => getAngleX(mp, self));
  mp.registerPapyrusFunction('method', 'Actor', 'GetAngleY', (self) => getAngleY(mp, self));
  mp.registerPapyrusFunction('method', 'Actor', 'GetAngleZ', (self) => getAngleZ(mp, self));
};
