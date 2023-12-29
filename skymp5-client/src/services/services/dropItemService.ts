import { Actor, ContainerChangedEvent } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";

// TODO: refactor this out
import * as taffyPerkSystem from '../../sweetpie/taffyPerkSystem';

import { MsgType } from "../../messages";
import { getWcProtection } from "../../features/worldCleaner";

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
        if (!this.sp.Ui.isMenuOpen("InventoryMenu"))
            return;
        if (
            isPlayer &&
            isReference &&
            noContainer &&
            taffyPerkSystem.canDropOrPutItem(e.baseObj.getFormID())
        ) {
            const radius: number = 2000;
            const baseId = e.baseObj.getFormID();

            const player = this.sp.Game.getPlayer() as Actor;

            let set = new Set<number>();
            for (let i = 0; i < 200; i++) {
                const refrId = this.sp.Game.findRandomReferenceOfType(
                    this.sp.Game.getFormEx(baseId),
                    player.getPositionX(),
                    player.getPositionY(),
                    player.getPositionZ(),
                    radius
                )?.getFormID();
                if (refrId) {
                    set.add(refrId);
                }
            }

            let numFound = 0;

            set.forEach((refrId) => {
                const ref = this.sp.ObjectReference.from(this.sp.Game.getFormEx(refrId));
                if (ref !== null && ref.isDeleted() === false) {
                    const refrId = ref.getFormID();
                    if (getWcProtection(refrId) === 0) {
                        ref.delete();
                        ++numFound;
                        this.logTrace("Found and deleted reference " + refrId.toString(16));
                    }
                    else
                        this.logTrace("Found reference " + refrId.toString(16) + " but it's protected");
                }
            });

            if (!numFound) {
                return this.logTrace("Ignoring item drop as false positive");
            }

            const t = MsgType.DropItem;
            const count = e.numItems;
            this.controller.emitter.emit("sendMessage", {
                message: {
                    t, baseId, count
                },
                reliability: "reliable"
            });
        }
    }
}
