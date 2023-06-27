import { Actor, Ammo, Game, Ui, setInventory } from 'skyrimPlatform';

import { Entry, Inventory, applyInventory, getInventory } from './inventory';

export interface Equipment {
  inv: Inventory;
  numChanges: number;
}

const filterWorn = (inv: Inventory): Inventory => {
  return { entries: inv.entries.filter((x) => x.worn || x.wornLeft) };
};

const removeUnnecessaryExtra = (inv: Inventory): Inventory => {
  return {
    entries: inv.entries.map((x) => {
      const r: Entry = JSON.parse(JSON.stringify(x));
      r.chargePercent = r.maxCharge;
      r.count = Ammo.from(Game.getFormEx(x.baseId)) ? 1000 : 1;
      delete r.name;
      return r;
    }),
  };
};

export const getEquipment = (ac: Actor, numChanges: number): Equipment => {
  return { inv: getInventory(ac), numChanges };
};

export const applyEquipment = (ac: Actor, eq: Equipment): boolean => {
  ac.removeAllItems(null, false, true);
  setInventory(ac.getFormID(), removeUnnecessaryExtra(filterWorn(eq.inv)));
  return true;
};

export const isBadMenuShown = (): boolean => {
  return (
    Ui.isMenuOpen('InventoryMenu') ||
    Ui.isMenuOpen('FavoritesMenu') ||
    Ui.isMenuOpen('MagicMenu') ||
    Ui.isMenuOpen('ContainerMenu') ||
    Ui.isMenuOpen('Crafting Menu') // Actually I don't think it causes crashes
  );
};
