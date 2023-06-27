import * as sp from 'skyrimPlatform';

import * as exp from '../sync/expSystem';
import { BasicEntry, Entry } from '../sync/inventory';

const keywordPerkMap = new Map<string, number>([
  // Стрелец
  ['SweetPerkBow1', 0x1036f0],
  ['SweetPerkBow2', 0x58f61],
  ['SweetPerkBow3', 0x105f19],
  ['SweetPerkBow4', 0x58f63],
  // Убийца
  ['SweetPerkSneak1', 0x58210],
  ['SweetPerkSneak2', 0x105f23],
  ['SweetPerkSneak3', 0x58208],
  ['SweetPerkSneak4', 0x58211],
  // Пехотинец
  ['SweetPerkArmorLight1', 0x105f22],
  ['SweetPerkArmorLight2', 0x51b1c],
  // Зачарователь
  ['SweetPerkEnchant2', 0x108a44],
  ['SweetPerkEnchant1', 0x58f7c],
  // Латник
  ['SweetPerkArmorHeavy1', 0xbcd2b],
  ['SweetPerkArmorHeavy2', 0x58f6d],
  // Щитоносец
  ['SweetPerkBlock1', 0x58f67],
  ['SweetPerkBlock2', 0x106253],
  ['SweetPerkBlock3', 0x58f6a],
  ['SweetPerkBlock4', 0x58f69],
  // Мечник, клинок, рубака, крушитель, монах, копейщик, стражник (двуручное оружие)
  ['SweetPerk1Hand2Hand1', 0x58f6f],
  ['SweetPerk2Hand2', 0xcb407],
  ['SweetPerk2Hand3', 0x96590],
  ['SweetPerk2Hand4', 0x52d51],
  // Ассасин, разведчик, шпион, предшественник, рыцарь, разбойник, берсерк, громила (одноручное оружие)
  ['SweetPerk1Hand2Hand1', 0x58f6f],
  ['SweetPerk1Hand2', 0xcb406],
  ['SweetPerk1Hand3', 0x106256],
  ['SweetPerk1Hand4', 0x52d50],
]);
const cantDropKeyword = 'SweetCantDrop';
const playerId = 0x14;

export const inventoryChanged = (
  refr: sp.ObjectReference,
  entry: BasicEntry,
): void => {
  // Only for player
  if (refr.getFormID() !== playerId) return;

  const item = sp.Game.getFormEx(entry.baseId);
  if (!item) return;

  if (entry.count > 0) {
    onItemAdded(item);
  }
};

export const canDropOrPutItem = (itemId: number): boolean => {
  return (
    !sp.Game.getFormEx(itemId)?.hasKeyword(
      sp.Keyword.getKeyword(cantDropKeyword),
    ) ?? true
  );
};

const onItemAdded = (item: sp.Form): void => {
  const perkIds = getPerkIdsByKeyword(item);
  if (perkIds) {
    exp.addPerk([playerId], perkIds);
  }
};

const getPerksByKeyword = (item: sp.Form): sp.Perk[] | null => {
  const result = new Array<sp.Perk>();
  keywordPerkMap.forEach((v, k) => {
    const keyWord = sp.Keyword.getKeyword(k);
    if (item.hasKeyword(keyWord)) {
      const perk = sp.Perk.from(sp.Game.getFormEx(v));
      if (perk) result.push(perk);
    }
  });

  return result.length > 0 ? result : null;
};

const getPerkIdsByKeyword = (item: sp.Form): number[] | null => {
  const result = new Array<number>();
  keywordPerkMap.forEach((v, k) => {
    const keyWord = sp.Keyword.getKeyword(k);
    if (item.hasKeyword(keyWord)) {
      result.push(v);
    }
  });

  return result.length > 0 ? result : null;
};
