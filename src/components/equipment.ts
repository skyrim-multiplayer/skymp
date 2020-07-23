import { Actor, Armor, Game } from 'skyrimPlatform';

export interface Equipment {
    armor: number[],
    numChanges: number
};

export let getEquipment = (actor: Actor, numChanges: number): Equipment => {
    let armor = new Array<number>();
    let n = actor.getNumItems();
    for (let i = 0; i < n; ++i) {
        let f = actor.getNthForm(i);
        if (Armor.from(f) && actor.isEquipped(f)) {
            armor.push(f.getFormID());
        }
    }
    return { armor, numChanges };
};

export let applyEquipment = (actor: Actor, equipment: Equipment) => {
    equipment.armor
        .map(id => Armor.from(Game.getFormEx(id)))
        .filter(form => !!form)
        .forEach(form => actor.equipItem(form, true, true));

    getEquipment(actor, -1).armor.forEach(armor => {
        if (equipment.armor.indexOf(armor) === -1) {
            let f = Armor.from(Game.getFormEx(armor));
            if (f) actor.unequipItem(f, true, true);
        }
    });
};