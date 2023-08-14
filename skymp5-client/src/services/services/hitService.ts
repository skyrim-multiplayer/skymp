import { HitEvent } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { MsgType } from "../../messages";
import { getHitData } from "../../sync/hit";
import { SkympClient } from "./skympClient";

export class HitService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
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
            const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;

            const sendTarget = skympClient.getSendTarget();
            if (sendTarget === undefined) {
                return this.logError("sendTarget was undefined in on('hit')");
            }
            sendTarget.send({ t: MsgType.OnHit, data: getHitData(e) }, true);
        }
    }

    // TODO: redirect this to spdlog
    private logError(error: string) {
        this.sp.printConsole("Error in HitService:", error);
    }

    // TODO: redirect this to spdlog
    private logTrace(trace: string) {
        this.sp.printConsole("Trace in HitService:", trace);
    }
}
