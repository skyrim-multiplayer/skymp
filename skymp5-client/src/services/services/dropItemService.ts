import { Actor, ContainerChangedEvent } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";

// TODO: refactor this out
import * as taffyPerkSystem from '../../sweetpie/taffyPerkSystem';

import { MsgType } from "../../messages";

export class DropItemService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        controller.on('containerChanged', (e) => this.onContainerChanged(e));
    }

    private onContainerChanged(e: ContainerChangedEvent) {
        const pl = this.sp.Game.getPlayer() as Actor;
        const isPlayer: boolean =
            pl && e.oldContainer && pl.getFormID() === e.oldContainer.getFormID();
        const noContainer: boolean =
            e.newContainer === null || e.newContainer === undefined;
        const isReference: boolean = e.reference !== null;
        if (e.newContainer && e.newContainer.getFormID() === pl.getFormID())
            return;
        if (
            isPlayer &&
            isReference &&
            noContainer &&
            taffyPerkSystem.canDropOrPutItem(e.baseObj.getFormID())
        ) {
            const radius: number = 200;
            const baseId: number = e.baseObj.getFormID();
            const refrId = this.sp.Game.findClosestReferenceOfType(
                e.baseObj,
                pl.getPositionX(),
                pl.getPositionY(),
                pl.getPositionZ(),
                radius,
            )?.getFormID();
            if (refrId) {
                const refr = this.sp.ObjectReference.from(this.sp.Game.getFormEx(refrId));
                if (refr) {
                    refr.delete().then(() => {
                        // TODO: handle possible exceptions in this function
                        const t = MsgType.DropItem;
                        const count = 1;
                        this.controller.emitter.emit("sendMessage", {
                            message: {
                                t, baseId, count
                            },
                            reliability: "reliable"
                        });
                    });
                }
            }
        }
    }
}
