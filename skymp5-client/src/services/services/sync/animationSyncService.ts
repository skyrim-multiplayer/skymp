
import { ClientListener, CombinedController, Sp } from "../clientListener";
import { UpdateAnimationMessage } from "../../../services/messages/updateAnimationMessage";
import { RemoteServer } from "../remoteServer";
import { applyAnimation } from "../../../sync/animation";
import { getObjectReference } from "../../../view/worldViewMisc";
import { FormViewFirstApplyAllEvent } from "../../../services/events/formViewFirstApplyAllEvent";
import { UniversalTickService } from "../universalTickService";
import { ConnectionMessage } from "../../../services/events/connectionMessage";

export class AnimationSyncService extends ClientListener {

    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        this.controller.emitter.on("updateAnimationMessage", (e) => this.onUpdateAnimationMessage(e));
        this.controller.emitter.on("formViewFirstApplyAll", (e) => this.onFormViewFirstApplyAll(e));

        this.controller.on("tick", () => this.onTick());
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
            const refr = /*id === remoteServer.getMyActorIndex() ? this.sp.Game.getPlayer() :*/ getObjectReference(id);
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
