import * as sp from "skyrimPlatform";
import { BasicEntry, Entry } from "../sync/inventory";
import * as exp from "../sync/expSystem";

const keywordPerkMap = new Map<string, number>([
  // Стрелец
  ["SweetPerkBow1", 0x1036F0],
  ["SweetPerkBow2", 0x58F61],
  ["SweetPerkBow3", 0x105F19],
  ["SweetPerkBow4", 0x58F63],
  // Убийца
  ["SweetPerkSneak1", 0x58210],
  ["SweetPerkSneak2", 0x105F23],
  ["SweetPerkSneak3", 0x58208],
  ["SweetPerkSneak4", 0x58211],
  // Пехотинец
  ["SweetPerkArmorLight1", 0x105F22],
  ["SweetPerkArmorLight2", 0x51B1C],
  // Зачарователь
  ["SweetPerkEnchant2", 0x108A44],
  ["SweetPerkEnchant1", 0x58F7C],
  // Латник
  ["SweetPerkArmorHeavy1", 0xBCD2B],
  ["SweetPerkArmorHeavy2", 0x58F6D],
  // Щитоносец
  ["SweetPerkBlock1", 0x58F67],
  ["SweetPerkBlock2", 0x106253],
  ["SweetPerkBlock3", 0x58F6A],
  ["SweetPerkBlock4", 0x58F69],
  // Мечник, клинок, рубака, крушитель, монах, копейщик, стражник (двуручное оружие)
  ["SweetPerk1Hand2Hand1", 0x58F6F],
  ["SweetPerk2Hand2", 0xCB407],
  ["SweetPerk2Hand3", 0x96590],
  ["SweetPerk2Hand4", 0x52D51],
  // Ассасин, разведчик, шпион, предшественник, рыцарь, разбойник, берсерк, громила (одноручное оружие)
  ["SweetPerk1Hand2Hand1", 0x58F6F],
  ["SweetPerk1Hand2", 0xCB406],
  ["SweetPerk1Hand3", 0x106256],
  ["SweetPerk1Hand4", 0x52D50],
]);
const cantDropKeyword = "SweetCantDrop";
const playerId = 0x14;

export const inventoryChanged = (refr: sp.ObjectReference, entry: BasicEntry): void => {
  // Only for player
  if (refr.getFormID() !== playerId) return;

  const item = sp.Game.getFormEx(entry.baseId);
  if (!item) return;

  if (entry.count > 0) {
    onItemAdded(item);
  }
}

export const canDropOrPutItem = (itemId: number): boolean => {
  return !sp.Game.getFormEx(itemId)?.hasKeyword(sp.Keyword.getKeyword(cantDropKeyword)) ?? true;
}

const onItemAdded = (item: sp.Form): void => {
  const perkIds = getPerkIdsByKeyword(item);
  if (perkIds) {
    exp.addPerk([playerId], perkIds);
  }
}

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
}

const getPerkIdsByKeyword = (item: sp.Form): number[] | null => {
  const result = new Array<number>();
  keywordPerkMap.forEach((v, k) => {
    const keyWord = sp.Keyword.getKeyword(k);
    if (item.hasKeyword(keyWord)) {
      result.push(v);
    }
  });

  return result.length > 0 ? result : null;
}
