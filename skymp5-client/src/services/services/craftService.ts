// TODO: refactor this out
import { localIdToRemoteId } from "../../view/worldViewMisc";

import { Actor, ContainerChangedEvent } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { Inventory } from "../../sync/inventory";
import { MsgType } from "../../messages";
import { SkympClient } from "./skympClient";

type FurnitureId = number;

export class CraftService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        controller.on('containerChanged', (e) => this.onContainerChanged(e));
    }

    private onContainerChanged(e: ContainerChangedEvent) {
        const oldContainerId = e.oldContainer ? e.oldContainer.getFormID() : 0;
        const newContainerId = e.newContainer ? e.newContainer.getFormID() : 0;
        const baseObjId = e.baseObj ? e.baseObj.getFormID() : 0;
        if (oldContainerId !== 0x14 && newContainerId !== 0x14) return;

        const furnitureRef = (this.sp.Game.getPlayer() as Actor).getFurnitureReference();
        if (!furnitureRef) return;

        const furnitureId = furnitureRef.getFormID();

        if (oldContainerId === 0x14 && newContainerId === 0) {
            let craftInputObjects = this.furnitureStreak.get(furnitureId);
            if (!craftInputObjects) {
                craftInputObjects = { entries: [] };
            }
            craftInputObjects.entries.push({
                baseId: baseObjId,
                count: e.numItems,
            });
            this.furnitureStreak.set(furnitureId, craftInputObjects);
            this.logTrace(
                `Adding baseObjId=${baseObjId.toString(16)}, numItems=${e.numItems} to craft`,
            );
        } else if (oldContainerId === 0 && newContainerId === 0x14) {
            this.logTrace('Finishing craft');
            const craftInputObjects = this.furnitureStreak.get(furnitureId);
            if (craftInputObjects && craftInputObjects.entries.length) {
                this.furnitureStreak.delete(furnitureId);
                const workbench = localIdToRemoteId(furnitureId);
                if (!workbench) {
                    return this.logError(`localIdToRemoteId returned 0 for furnitureId=${furnitureId}`);
                }

                const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;
                const sendTarget = skympClient.getSendTarget();
                if (sendTarget === undefined) {
                    return this.logError("sendTarget was undefined in onActivate");
                }

                const resultObjectId = baseObjId;

                this.logTrace(`Sending craft workbench=${workbench}, resultObjectId=${resultObjectId}, craftInputObjects=${JSON.stringify(craftInputObjects.entries)}`);

                sendTarget.send(
                    {
                        t: MsgType.CraftItem,
                        data: { workbench, craftInputObjects, resultObjectId },
                    },
                    true,
                );
            }
        }
    }

    // TODO: redirect this to spdlog
    private logError(error: string) {
        this.sp.printConsole("Error in CraftService:", error);
    }

    // TODO: redirect this to spdlog
    private logTrace(trace: string) {
        this.sp.printConsole("Trace in CraftService:", trace);
    }

    private furnitureStreak = new Map<FurnitureId, Inventory>();
}
