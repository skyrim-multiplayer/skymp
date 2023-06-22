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
                // Hot reload is not supported for now
                // Just stop the loop here
                ctx.sp.storage.sweetCarryAnimationActive = false;
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
                // Uncomment this to test with any items:
                // return true;
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

            const startCarry = (baseObj?: Form) => {
                ctx.sp.Debug.sendAnimationEvent(ctx.sp.Game.getPlayer(), SWEET_CARRY_ANIM_NAME);
                    ctx.sp.storage.sweetCarryEquippedFormId = baseObj?.getFormID() ?? null;
                    ctx.sp.storage.sweetCarryAnimationActive = true;
                    (function () {
                        function sendAnimation() {
                          // This will not succeed, but still will be sent to the server (see workaround in skymp5-client/animation.ts)
                          // Make sure that people will spawn with animation for others
                          if (ctx.sp.storage.sweetCarryAnimationActive) {
                            ctx.sp.Debug.sendAnimationEvent(ctx.sp.Game.getPlayer(), SWEET_CARRY_ANIM_NAME);
                          }
                        }
                      
                        function checkAnimation() {
                          if (ctx.sp.storage.sweetCarryAnimationActive) {
                            ctx.sp.Utility.wait(3).then(() => {
                              sendAnimation();
                              checkAnimation();
                            });
                          }
                        }
                      
                        checkAnimation();
                    })();
            };

            // Restore carry animation on game reconnect
            ctx.sp.once("update", () => {
                let startCarry2 = startCarry;
                for (let j = 1; j < 5; ++j) {
                    ctx.sp.Utility.wait(j).then(() => {
                        const pl = ctx.sp.Game.getPlayer();
                        const n = pl?.getNumItems();
                        if (n) {
                            for (let i = 0; i < n; i++) {
                                const form = pl?.getNthForm(i) || null;
                                if (pl?.isEquipped(form) && form !== null && hasKeyword(form)) {
                                    startCarry2(form);
                                    startCarry2 = () => {};
                                    return;
                                }
                            }
                        }
                    });
                }
            });

            ctx.sp.on("equip", (event) => {
                if (event.actor.getFormID() == playerId && hasKeyword(event.baseObj)) {
                    startCarry(event.baseObj);
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
