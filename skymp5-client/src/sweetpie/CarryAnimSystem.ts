import * as sp from "skyrimPlatform";

const playerId = 0x14;

const SWEET_CARRY_ANIM_KEYWORD = "SweetAnimationCarry";
const SWEET_CARRY_ANIM_NAME = "OffsetCarryBasketStart";
const SWEET_CARRY_ANIM_RESET = "IdleForceDefaultState";
const SWEET_CARRY_ANIM_RESTRICT = [
    "Jump*",
    "JampStandingStart",
    "JampLand",
    "JampFall",
    "JumpDirectionalStart",
    "JumpLandDirectional",
];
let SweetCarryAnimationActive = false;
let SweetCarryEquippedFormId: number | null = null;

export function Install() {
    sp.hooks.sendAnimationEvent.add({
        enter: ((ctx) => {
            if (SweetCarryAnimationActive) {
                ctx.animEventName = "";
            }
        }),
        leave: () => { },
    }, playerId, playerId, SWEET_CARRY_ANIM_RESTRICT[0]);

    sp.on("equip", (event: sp.EquipEvent) => {
        if (event.actor.getFormID() == playerId && hasKeyword(event.baseObj)) {
            sp.Debug.sendAnimationEvent(sp.Game.getPlayer(), SWEET_CARRY_ANIM_NAME);
            SweetCarryEquippedFormId = event.originalRefr.getFormID();
            SweetCarryAnimationActive = true;
        }
    });

    sp.on("unequip", (event: sp.EquipEvent) => {
        if (event.actor.getFormID() == playerId && hasKeyword(event.baseObj)) {
            SweetCarryAnimationActive = false;
            SweetCarryEquippedFormId = null;
            sp.Debug.sendAnimationEvent(sp.Game.getPlayer(), SWEET_CARRY_ANIM_RESET);
        }
    });

    sp.on("combatState", (event: sp.CombatEvent) => {
        if (event.isCombat && event.actor.getFormID() == playerId && SweetCarryAnimationActive == true && SweetCarryEquippedFormId) {
            const unqForm = sp.Game.getFormEx(SweetCarryEquippedFormId);
            sp.Actor.from(event.actor)?.unequipItem(unqForm, false, false);
        }
    });
}

function hasKeyword(form: sp.Form): boolean {
    const kw1 = sp.Keyword.getKeyword(SWEET_CARRY_ANIM_KEYWORD);
    return (kw1 && form.hasKeyword(kw1)) ?? false;
}
