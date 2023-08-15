import { Actor, ContainerChangedEvent, Game, printConsole } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { MsgType } from "../../messages";
import { getPcInventory } from "../../modelSource/remoteServer";
import { getInventory, getDiff, hasExtras, removeSimpleItemsAsManyAsPossible, sumInventories } from "../../sync/inventory";
import { LastInvService } from "./lastInvService";

// TODO: refactor this out
import * as taffyPerkSystem from '../../sweetpie/taffyPerkSystem';

import { SkympClient } from "./skympClient";

export class ContainersService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        controller.on('containerChanged', (e) => this.onContainerChanged(e));
    }

    private onContainerChanged(e: ContainerChangedEvent) {
        if (e.oldContainer && e.newContainer) {
            if (
                e.oldContainer.getFormID() === 0x14 ||
                e.newContainer.getFormID() === 0x14
            ) {
                if (e.newContainer.getFormID() === 0x14 && e.numItems > 0) {
                    taffyPerkSystem.inventoryChanged(e.newContainer, {
                        baseId: e.baseObj.getFormID(),
                        count: e.numItems,
                    });
                }

                const lastInvService = this.controller.lookupListener("LastInvService") as LastInvService;

                if (!lastInvService.lastInv) lastInvService.lastInv = getPcInventory();
                if (lastInvService.lastInv) {
                    const newInv = getInventory(Game.getPlayer() as Actor);

                    // It seems that 'ignoreWorn = false' fixes this:
                    // https://github.com/skyrim-multiplayer/issue-tracker/issues/43
                    // For some reason excess diff is produced when 'ignoreWorn = true'
                    // I thought that it would be vice versa but that's how it works
                    const ignoreWorn = false;
                    const diff = getDiff(lastInvService.lastInv, newInv, ignoreWorn);

                    printConsole('diff:');
                    for (let i = 0; i < diff.entries.length; ++i) {
                        printConsole(`[${i}] ${JSON.stringify(diff.entries[i])}`);
                    }
                    const msgs = diff.entries
                        .filter((entry) =>
                            entry.count > 0
                                ? taffyPerkSystem.canDropOrPutItem(entry.baseId)
                                : true,
                        )
                        .map((entry) => {
                            if (entry.count !== 0) {
                                const msg = JSON.parse(JSON.stringify(entry));
                                if (Game.getFormEx(entry.baseId)?.getName() === msg['name']) {
                                    delete msg['name'];
                                }
                                msg['t'] =
                                    entry.count > 0 ? MsgType.PutItem : MsgType.TakeItem;
                                msg['count'] = Math.abs(msg['count']);
                                msg['target'] =
                                    e.oldContainer.getFormID() === 0x14
                                        ? e.newContainer.getFormID()
                                        : e.oldContainer.getFormID();
                                return msg;
                            }
                        });


                    const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;
                    const sendTarget = skympClient.getSendTarget();
                    if (sendTarget === undefined) {
                        this.logError("sendTarget was undefined in on('containerChanged')");
                    }
                    else {
                        msgs.forEach((msg) => sendTarget.send(msg, true));
                    }

                    // Prevent emitting 1,2,3,4,5 changes when taking/putting 5 potions one by one
                    // This code makes it 1,1,1,1,1 but works only for extra-less items
                    // At the moment of writing this I think it's not needed for items with extras
                    diff.entries.forEach((entry) => {
                        if (lastInvService.lastInv && !hasExtras(entry)) {
                            const put = entry.count > 0;
                            const take = entry.count < 0;
                            if (put) {
                                lastInvService.lastInv = removeSimpleItemsAsManyAsPossible(
                                    lastInvService.lastInv,
                                    entry.baseId,
                                    entry.count,
                                );
                            } else if (take) {
                                const add = { entries: [entry] };
                                add.entries[0].count *= -1;
                                lastInvService.lastInv = sumInventories(lastInvService.lastInv, add);
                            }
                        }
                    });
                }
            }
        }
    }
}
