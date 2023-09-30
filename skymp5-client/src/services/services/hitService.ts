// TODO: refactor this out
import { localIdToRemoteId } from "../../view/worldViewMisc";

import { FormType, HitEvent, storage } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { MsgType } from "../../messages";
import { Hit } from "../messages/hitMessage";

export class HitService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        controller.on('hit', (e) => this.onHit(e));
    }

    private onHit(e: HitEvent) {
        // TODO: add more logging in case of 'return'
        // TODO: allow non-weapon sources
        const aggressor = e.aggressor.getFormID();
        if (aggressor < 0xff000000 && aggressor !== 0x14) return; // all skymp npcs are FF+

        // TODO: make host service
        const hosted = storage['hosted'];
        let alreadyHosted = false;
        if (Array.isArray(hosted)) {
          const remoteId = localIdToRemoteId(aggressor);
          if (hosted.includes(remoteId)) {
            alreadyHosted = true;
          }
        }

        if (!alreadyHosted) return;

        const base = e.target.getBaseObject();
        const type = base?.getType();

        if (type === FormType.Static || type === FormType.MovableStatic) {
            return;
        }

        if (this.sp.Weapon.from(e.source)) {
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
