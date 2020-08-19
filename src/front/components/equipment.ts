import { Actor, Game, Ammo, Ui } from "skyrimPlatform";
import { Inventory, getInventory, applyInventory, Entry } from "./inventory";

import * as structures from "../../lib/structures/equipment";
export type Equipment = structures.Equipment;

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
  return applyInventory(ac, removeUnnecessaryExtra(filterWorn(eq.inv)), true);
};

export const isBadMenuShown = (): boolean => {
  return (
    Ui.isMenuOpen("InventoryMenu") ||
    Ui.isMenuOpen("FavoritesMenu") ||
    Ui.isMenuOpen("MagicMenu") ||
    Ui.isMenuOpen("ContainerMenu")
  );
};
