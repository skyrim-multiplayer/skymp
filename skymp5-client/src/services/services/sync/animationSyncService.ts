
import { ClientListener, CombinedController, Sp } from "../clientListener";
import { UpdateAnimationMessage } from "../../../services/messages/updateAnimationMessage";
import { RemoteServer } from "../remoteServer";
import { applyAnimation } from "../../../sync/animation";
import { getObjectReference } from "../../../view/worldViewMisc";
import { FormViewFirstApplyAllEvent } from "../../../services/events/formViewFirstApplyAllEvent";
import { UniversalTickService } from "../universalTickService";
import { ConnectionMessage } from "../../../services/events/connectionMessage";
import { printConsole, SendAnimationEventHook } from "skyrimPlatform";
import { MsgType } from "../../../messages";
import { MessageWithRefrId } from "../../../services/events/sendMessageWithRefrIdEvent";

// TODO: refactor this out
import * as worldViewMisc from "../../../view/worldViewMisc";

export class AnimationSyncService extends ClientListener {

    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        this.controller.emitter.on("updateAnimationMessage", (e) => this.onUpdateAnimationMessage(e));
        this.controller.emitter.on("formViewFirstApplyAll", (e) => this.onFormViewFirstApplyAll(e));

        this.controller.on("tick", () => this.onTick());

        this.sp.hooks.sendAnimationEvent.add({
            enter: () => { },
            leave: (ctx) => { this.onSendAnimationEventLeave(ctx); },
        });
    }

    private onSendAnimationEventLeave(ctx: SendAnimationEventHook.LeaveContext) {
        const hostedUnknown = this.sp.storage['hosted'];
        const hosted: unknown[] = Array.isArray(hostedUnknown) ? hostedUnknown : [];
        const _refrId: number | undefined = ctx.selfId === 0x14 ? undefined : worldViewMisc.localIdToRemoteId(ctx.selfId);

        // Do not further process if not us and not our hosted refrs
        if (_refrId === 0) {
            // localIdToRemoteId returns 0 for unknown refs
            return;
        }
        if (typeof _refrId === "number" && !hosted.includes(_refrId) && !hosted.includes(_refrId + 0x100000000)) {
            return;
        }

        let animEventName = ctx.animEventName;

        // Skip non-successful animations
        if (!ctx.animationSucceeded) {
            // Workaround, see carryAnimSystem.ts in gamemode
            // Case-sensetive check here for better performance
            if (animEventName !== "OffsetCarryBasketStart") {
                return;
            }
        }

        if (animEventName === "moveStart"
            || animEventName === "moveStop"
            || animEventName === "turnStop"
            || animEventName === "CyclicCrossBlend"
            || animEventName === "CyclicFreeze"
            || animEventName === "TurnLeft"
            || animEventName === "TurnRight") {
            return;
        }

        const lower = animEventName.toLowerCase();

        const isTorchEvent = lower.includes("torch");
        if (lower.includes("unequip") && !isTorchEvent) {
            animEventName = "SkympFakeUnequip";
        } else if (lower.includes("equip") && !isTorchEvent) {
            animEventName = "SkympFakeEquip";
        }

        // Drink potion anim from this mod https://www.nexusmods.com/skyrimspecialedition/mods/97660
        if (animEventName !== '' && !animEventName.startsWith("DrinkPotion_")) {
            //this.updateActorValuesAfterAnimation(animEventName);
            const message: MessageWithRefrId<UpdateAnimationMessage> = {
                t: MsgType.UpdateAnimation,
                data: {
                    animEventName: animEventName,
                    numChanges: 0
                },
                _refrId
            };
            this.controller.emitter.emit("sendMessageWithRefrId", {
                message,
                reliability: "unreliable"
            });
        }
    }

    private onTick() {
        const currentTick = performance.now();
        const delta = currentTick - this.lastTick;
        this.lastTick = currentTick;

        this.deltas.push(delta);

        if (this.deltas.length > 30) {
            this.deltas.shift();
        }

        const fpsFromDeltas = 1000 / (this.deltas.reduce((a, b) => a + b, 0) / this.deltas.length || 1);
        //this.sp.printConsole(`FPS: ${fpsFromDeltas.toFixed(2)}`);
    }

    private lastTick: number = 0;
    private deltas: number[] = [];

    private onUpdateAnimationMessage(event: ConnectionMessage<UpdateAnimationMessage>): void {
        const msg = event.message;

        const remoteServer = this.controller.lookupListener(RemoteServer);
        const universalTickService = this.controller.lookupListener(UniversalTickService);
        const idManager = remoteServer.getIdManager();
        const worldModel = remoteServer.getWorldModel();

        // Save data for later
        const i = idManager.getId(msg.idx);
        const form = worldModel.forms[i];

        if (form === undefined) {
            //logError(this, `onUpdateAnimationMessage - Form with idx`, msg.idx, `not found`);
            return;
        }

        form.animation = msg.data;

        // Apply immediately
        const apply = () => {
            const id = ("idx" in msg && typeof msg.idx === "number") ? idManager.getId(msg.idx) : remoteServer.getMyActorIndex();
            const refr = getObjectReference(id);
            const actor = this.sp.Actor.from(refr);

            if (!actor) {
                //logTrace(this, `onUpdateAnimationMessage - Actor with idx`, msg.idx, `not found or is not an Actor`);
                return;
            }

            applyAnimation(actor, msg.data, { useAnimOverrides: false });
        };

        if (universalTickService.getCurrentUniversalTickType() === "update") {
            apply();
        } else {
            this.controller.once("update", apply);
        }
    }

    private onFormViewFirstApplyAll(event: FormViewFirstApplyAllEvent): void {
        const { refr, model } = event;

        if (!model.animation) {
            return;
        }

        const actor = this.sp.Actor.from(refr);
        if (!actor) {
            //logTrace(this, `onFormViewFirstApplyAll - refr is not an Actor`);
            return;
        }

        applyAnimation(actor, model.animation, { useAnimOverrides: true });
    }
}
