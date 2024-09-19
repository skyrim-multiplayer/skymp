import { logTrace } from "../../logging";
import { ClientListener, Sp, CombinedController } from "./clientListener";

export class BlockedAnimationsService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        const blockedAnims = ["IdleNocturnal*", "IdleCarryBucketFillEnter", "IdleGreybeardMeditateEnter", "IdleWriteTableChairEnterInstant", "Idlelounge", "IdleTGFalmerStatueEnter"];

        const self = this;

        blockedAnims.forEach(blockedAnim => {
            this.sp.hooks.sendAnimationEvent.add({
                enter(ctx) {
                    logTrace(self, `blocking animation event`, ctx.animEventName);
                    ctx.animEventName = "";
                },
                leave() { }
            }, 0x14, 0x14, blockedAnim);
        });
    }
};
