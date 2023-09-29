// TODO: refactor this out
import { localIdToRemoteId } from "../../view/worldViewMisc";

import { HitEvent } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { MsgType } from "../../messages";
import { SkympClient } from "./skympClient";
import { Hit } from "../messages/hitMessage";

export class HitService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        controller.on('hit', (e) => this.onHit(e));
    }

    private onHit(e: HitEvent) {
        // TODO: add more logging in case of 'return'
        // TODO: allow npcs to attack
        // TODO: allow non-weapon sources
        // TODO: allow non-actor targets
        const playerFormId = 0x14;
        if (e.target.getFormID() === playerFormId) return;
        if (e.aggressor.getFormID() !== playerFormId) return;
        if (this.sp.Weapon.from(e.source) && this.sp.Actor.from(e.target)) {
            this.controller.emitter.emit("sendMessage", {
                message: { t: MsgType.OnHit, data: this.getHitData(e) },
                reliability: "reliable"
            });
        }
    }

    private getHitData(e: HitEvent): Hit {
        const hitData: Hit = {
            aggressor: localIdToRemoteId(e.aggressor.getFormID()),
            isBashAttack: e.isBashAttack,
            isHitBlocked: e.isHitBlocked,
            isPowerAttack: e.isPowerAttack,
            isSneakAttack: e.isSneakAttack,
            projectile: e.projectile ? e.projectile.getFormID() : 0,
            source: e.source ? e.source.getFormID() : 0,
            target: localIdToRemoteId(e.target.getFormID())
        }
        return hitData;
    }
}
