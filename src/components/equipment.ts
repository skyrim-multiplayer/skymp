import { Actor, Armor, Game, Ammo, Form, Light, Utility, printConsole, on } from 'skyrimPlatform';
import * as sp from 'skyrimPlatform';

export interface Equipment {
    armor: number[],
    numChanges: number
};

enum FormType {
    kLight = 31
};

let isArmorLike = (form: Form) => Armor.from(form) || Ammo.from(form) || Light.from(form);

export let isBadMenuShown = () => {
    let menus = [
        'InventoryMenu', 
        'FavoritesMenu',
        'MagicMenu'
    ];
    let isMenuOpen = sp['UI']['isMenuOpen'];
    return menus.map(name => isMenuOpen(name)).indexOf(true) !== -1;
}

let isAnyTorchEquipped = (actor: Actor) => {
    const torch = 11;
    const leftHand = 0;
    return actor.getEquippedItemType(leftHand) === torch;
}

let isEquipped = (actor: Actor, f: Form) => {
    //printConsole('isEquipped', f.getType());
    if (f.getType() === FormType.kLight) {
        return isAnyTorchEquipped(actor);
    }
    return actor.isEquipped(f);
}

let unequipItem = (actor: Actor, f: Form) => {
    //printConsole('unequipItem', f.getType());
    if (f.getType() === FormType.kLight) {
        if (isBadMenuShown()) return;
    }
    actor.unequipItem(f, true, true);
}

let equipItem = (actor: Actor, f: Form) => {
    let armor = Armor.from(f);
    const shieldMask = 0x00000200;
    let isShield = armor && armor.getSlotMask() & shieldMask;
    if (isShield || Light.from(f)) {
        if (isAnyTorchEquipped(actor) && isBadMenuShown()) {
            return printConsole(`sorry it may lead to crash`);
        }
    }
    actor.equipItem(f, true, true);
}

export let getEquipment = (actor: Actor, numChanges: number): Equipment => {
    let armor = new Array<number>();
    let n = actor.getNumItems();
    for (let i = 0; i < n; ++i) {
        let f = actor.getNthForm(i);
        if (isArmorLike(f) && isEquipped(actor, f)) {
            armor.push(f.getFormID());
        }
    }
    return { armor, numChanges };
};

export let applyEquipment = (actor: Actor, equipment: Equipment) => {
    getEquipment(actor, -1).armor.forEach(armor => {
        if (equipment.armor.indexOf(armor) === -1) {
            let f = Game.getFormEx(armor);
            if (f && isArmorLike(f)) unequipItem(actor, f);
        }
    });

    equipment.armor
        .map(id => Game.getFormEx(id))
        .filter(form => !!form)
        .forEach(form => equipItem(actor, form));
};