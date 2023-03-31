import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';

import { Form } from 'skyrimPlatform';

declare const mp: Mp;
declare const ctx: Ctx;

export class CarryAnimSystem {
    static init() {
        mp.makeEventSource('_onCarryAnimSystem', new FunctionInfo(this.clientsideInitEvent()).getText());

        // doesn't really emit anything, just calls the function
        mp['_onCarryAnimSystem'] = () => { };
    }

    private static clientsideInitEvent() {
        return () => {
            if (ctx.sp.storage["CarryAnimSystemInstalled"] !== true) {
                ctx.sp.storage["CarryAnimSystemInstalled"] = true;
            }
            else {
                return;
            }

            if (typeof ctx.sp.storage.sweetCarryAnimationActive !== "boolean") {
                ctx.sp.storage.sweetCarryAnimationActive = false;
            }
            if (typeof ctx.sp.storage.sweetCarryEquippedFormId !== "number"
                && ctx.sp.storage.sweetCarryEquippedFormId !== null) {
                ctx.sp.storage.sweetCarryEquippedFormId = null;
            }

            const playerId: number = 0x14;

            const SWEET_CARRY_ANIM_KEYWORD = "SweetAnimationCarry";
            const SWEET_CARRY_ANIM_NAME = "OffsetCarryBasketStart";
            const SWEET_CARRY_ANIM_RESET = "IdleForceDefaultState";
            const SWEET_CARRY_ANIM_RESTRICT = [
                "Jump*",
                "SprintStart",
                "WeapEquip"
            ];

            function hasKeyword(form: Form): boolean {
                const kw1 = ctx.sp.Keyword.getKeyword(SWEET_CARRY_ANIM_KEYWORD);
                return (kw1 && form.hasKeyword(kw1)) ?? false;
            }

            for (let restrictedAnim of SWEET_CARRY_ANIM_RESTRICT) {
                ctx.sp.hooks.sendAnimationEvent.add({
                    enter: ((animationEventCtx) => {
                        if (ctx.sp.storage.sweetCarryAnimationActive) {
                            animationEventCtx.animEventName = "";
                        }
                    }),
                    leave: () => { },
                }, playerId, playerId, restrictedAnim);
            }

            ctx.sp.on("equip", (event) => {
                if (event.actor.getFormID() == playerId && hasKeyword(event.baseObj)) {
                    ctx.sp.Debug.sendAnimationEvent(ctx.sp.Game.getPlayer(), SWEET_CARRY_ANIM_NAME);
                    ctx.sp.storage.sweetCarryEquippedFormId = event.baseObj?.getFormID() ?? null;
                    ctx.sp.storage.sweetCarryAnimationActive = true;
                }
            });

            ctx.sp.on("unequip", (event) => {
                if (event.actor.getFormID() == playerId && hasKeyword(event.baseObj)) {
                    ctx.sp.storage.sweetCarryAnimationActive = false;
                    ctx.sp.storage.sweetCarryEquippedFormId = null;
                    ctx.sp.Debug.sendAnimationEvent(ctx.sp.Game.getPlayer(), SWEET_CARRY_ANIM_RESET);
                }
            });

            ctx.sp.on("combatState", (event) => {
                if (event.isCombat && event.actor.getFormID() == playerId && ctx.sp.storage.sweetCarryAnimationActive == true && ctx.sp.storage.sweetCarryEquippedFormId) {
                    const formId = ctx.sp.storage.sweetCarryEquippedFormId;
                    if (typeof formId !== "number") {
                        return;
                    }
                    const unqForm = ctx.sp.Game.getFormEx(formId);
                    ctx.sp.Actor.from(event.actor)?.unequipItem(unqForm, false, false);
                }
            });
        };
    }
}
