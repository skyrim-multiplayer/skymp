import {
  Actor,
  Armor,
  Game,
  Ammo,
  Form,
  Light,
  Weapon,
  printConsole,
  Ui,
} from "skyrimPlatform";

export interface Equipment {
  armor: number[];
  numChanges: number;
  leftHandWeapon: number;
  rightHandWeapon: number;
}

enum FormType {
  kLight = 31,
}

const isArmorLike = (form: Form) =>
  Armor.from(form) || Ammo.from(form) || Light.from(form);
//const isWeaponLike = (form: Form) => Weapon.from(form);

export const isBadMenuShown = (): boolean => {
  const menus = ["InventoryMenu", "FavoritesMenu", "MagicMenu"];
  return menus.map((name) => Ui.isMenuOpen(name)).indexOf(true) !== -1;
};

const isAnyTorchEquipped = (actor: Actor) => {
  const torch = 11;
  const leftHand = 0;
  return actor.getEquippedItemType(leftHand) === torch;
};

const isEquipped = (actor: Actor, f: Form) => {
  //printConsole('isEquipped', f.getType());
  if (f.getType() === FormType.kLight) {
    return isAnyTorchEquipped(actor);
  }
  return actor.isEquipped(f);
};

const unequipItem = (actor: Actor, f: Form) => {
  //printConsole('unequipItem', f.getType());
  if (f.getType() === FormType.kLight) {
    if (isBadMenuShown()) return;
  }
  actor.unequipItem(f, true, true);
};

const isBothHandsWeapon = (weapon: Weapon) => {
  if (!weapon) return false;
  switch (weapon.getWeaponType()) {
    case 5:
    case 6:
    case 7:
    case 9:
      return true;
  }
  return false;
};

const equipItem = (actor: Actor, f: Form) => {
  const armor = Armor.from(f);
  const shieldMask = 0x00000200;
  const isShield = armor && armor.getSlotMask() & shieldMask;
  const weapon = Weapon.from(f);
  if (isShield || Light.from(f) || isBothHandsWeapon(weapon)) {
    if (isAnyTorchEquipped(actor) && isBadMenuShown()) {
      return printConsole("sorry it may lead to crash");
    }
  }
  const isStaff = weapon && weapon.getWeaponType() === 8;
  if (isStaff) return printConsole("staffs are disabled for now");
  actor.equipItem(f, true, true);
};

export const getEquippedWeaponId = (actor: Actor, isLeft: boolean): number => {
  const weapon = actor.getEquippedWeapon(isLeft);
  return weapon ? weapon.getFormID() : 0;
};

export const getEquipment = (actor: Actor, numChanges: number): Equipment => {
  const armor = new Array<number>();
  const n = actor.getNumItems();

  for (let i = 0; i < n; ++i) {
    const f = actor.getNthForm(i);
    if (isArmorLike(f) && isEquipped(actor, f)) {
      armor.push(f.getFormID());
    }
  }

  const rightHandWeapon = getEquippedWeaponId(Game.getPlayer(), false);
  const leftHandWeapon = getEquippedWeaponId(Game.getPlayer(), true);
  return { armor, numChanges, leftHandWeapon, rightHandWeapon };
};

export const applyEquipment = (actor: Actor, equipment: Equipment): void => {
  getEquipment(actor, -1).armor.forEach((armor) => {
    if (equipment.armor.indexOf(armor) === -1) {
      const f = Game.getFormEx(armor);
      if (f && isArmorLike(f)) unequipItem(actor, f);
    }
  });

  equipment.armor
    .map((id) => Game.getFormEx(id))
    .filter((form) => !!form)
    .forEach((form) => equipItem(actor, form));

  if (equipment.rightHandWeapon === 0) {
    const rWeap = getEquippedWeaponId(actor, false);
    if (rWeap !== 0) {
      actor.unequipItem(Game.getFormEx(rWeap), true, true);
    }
  }

  if (typeof equipment.rightHandWeapon === "number") {
    const right = Game.getFormEx(equipment.rightHandWeapon);
    if (
      right &&
      !isBadMenuShown() &&
      equipment.rightHandWeapon !== getEquippedWeaponId(actor, false)
    ) {
      equipItem(actor, right);
    }
  }

  /*if (false) {
            let idForLeft = cloneWeapoForLeftHand(equipment.leftHandWeapon);
            printConsole(`idForLeft  = ${idForLeft}`);
           actor.equipItem(Game.getFormEx(idForLeft), true, true);
    }*/
};
