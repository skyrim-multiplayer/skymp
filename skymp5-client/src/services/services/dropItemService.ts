import { Actor, ContainerChangedEvent } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";

import { MsgType } from "../../messages";
import { SweetTaffySweetCantDropService } from "./sweetTaffySweetCantDropService";
import { WorldCleanerService } from "./worldCleanerService";
import { logTrace } from "../../logging";

export class DropItemService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        controller.on('containerChanged', (e) => this.onContainerChanged(e));
    }

    private onContainerChanged(e: ContainerChangedEvent) {
        const sweetCantDropService = this.controller.lookupListener(SweetTaffySweetCantDropService);

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
            sweetCantDropService.canDropOrPutItem(e.baseObj.getFormID())
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
                else {
                    break;
                }
            }

            let numFound = 0;

            const worldCleanerService = this.controller.lookupListener(WorldCleanerService);

            set.forEach((refrId) => {
                const ref = this.sp.ObjectReference.from(this.sp.Game.getFormEx(refrId));
                if (ref !== null && ref.isDeleted() === false) {
                    const refrId = ref.getFormID();

                    if (worldCleanerService.getWcProtection(refrId) === 0) {
                        ref.delete();
                        ++numFound;
                        logTrace(this, "Found and deleted reference " + refrId.toString(16));
                    }
                    else
                        logTrace(this, "Found reference " + refrId.toString(16) + " but it's protected");
                }
            });

            if (!numFound) {
                return logTrace(this, "Ignoring item drop as false positive");
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
