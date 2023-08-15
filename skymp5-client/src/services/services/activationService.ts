import { ActivateEvent, Actor } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { MsgType } from "../../messages";
import { getInventory } from "../../sync/inventory";

// TODO: refactor this out
import { localIdToRemoteId } from "../../view/worldViewMisc";

import { SkympClient } from "./skympClient";
import { LastInvService } from "./lastInvService";

export class ActivationService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.on("activate", (e) => this.onActivate(e));
    }

    private onActivate(e: ActivateEvent) {
        const lastInvService = this.controller.lookupListener("LastInvService") as LastInvService;
        lastInvService.lastInv = getInventory(this.sp.Game.getPlayer() as Actor);

        let caster = e.caster ? e.caster.getFormID() : 0;
        let target = e.target ? e.target.getFormID() : 0;

        if (!target || !caster) return;

        // Actors never have non-ff ids locally in skymp
        if (caster !== 0x14 && caster < 0xff000000) return;

        target = localIdToRemoteId(target);
        if (!target) {
            return this.logError('localIdToRemoteId returned 0 (target) in on(\'activate\')');
        }

        caster = localIdToRemoteId(caster);
        if (!caster) {
            return this.logError('localIdToRemoteId returned 0 (caster) in on(\'activate\')');
        }

        const openState = e.target.getOpenState();

        // TODO: add this to skyrimPlatform.ts
        const enum OpenState {
            None,
            Open,
            Opening,
            Closed,
            Closing,
        }

        if (openState === OpenState.Opening || openState === OpenState.Closing) {
            return this.logTrace("Ignoring activation of door because it's already opening or closing");
        }

        const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;
        const sendTarget = skympClient.getSendTarget();
        if (sendTarget === undefined) {
            return this.logError("sendTarget was undefined in onActivate");
        }

        sendTarget.send(
            { t: MsgType.Activate, data: { caster, target } },
            true,
        );

        this.logTrace(`Sent activation for caster=${caster.toString(16)} and target=${target.toString(16)}`);
    }
};
