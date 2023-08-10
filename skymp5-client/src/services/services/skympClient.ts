import * as sp from 'skyrimPlatform';
import {
  Actor,
  Game,
  ObjectReference,
  on,
  once,
  printConsole,
  settings,
  storage,
} from 'skyrimPlatform';

import * as netInfo from '../../debug/netInfoSystem';
import * as updateOwner from '../../gamemodeApi/updateOwner';
import * as networking from '../../networking';
import * as taffyPerkSystem from '../../sweetpie/taffyPerkSystem';
import * as deathSystem from '../../sync/deathSystem';
import { setUpConsoleCommands } from '../../features/console';
import { HostStartMessage, HostStopMessage, MsgType } from '../../messages';
import { ModelSource } from '../../modelSource/modelSource';
import { MsgHandler } from '../../modelSource/msgHandler';
import { RemoteServer, getPcInventory } from '../../modelSource/remoteServer';
import { SendTarget } from '../../modelSource/sendTarget';
import { setupHooks } from '../../sync/animation';
import { getHitData } from '../../sync/hit';
import * as animDebugSystem from '../../debug/animDebugSystem';
import {
  Inventory,
  getDiff,
  getInventory,
  hasExtras,
  removeSimpleItemsAsManyAsPossible,
  sumInventories,
} from '../../sync/inventory';
import { WorldView } from '../../view/worldView';
import { localIdToRemoteId } from '../../view/worldViewMisc';
import { SinglePlayerService } from './singlePlayerService';
import { verifyVersion } from '../../version';
import * as authSystem from "../../features/authSystem";
import * as playerCombatSystem from "../../sweetpie/playerCombatSystem";
import { AuthGameData } from '../../features/authModel';
import { Transform } from '../../sync/movement';
import * as browser from "../../features/browser";
import { CombinedController, Sp } from './clientListener';

interface AnyMessage {
  type?: string;
  t?: number;
}

const handleMessage = (msgAny: AnyMessage, handler_: MsgHandler) => {
  const msgType: string = msgAny.type || (MsgType as any)[msgAny.t as any];
  const handler = handler_ as unknown as Record<
    string,
    (m: AnyMessage) => void
  >;
  const f = handler[msgType];

  if (msgType === 'hostStart') {
    const msg = msgAny as HostStartMessage;
    const target = msg.target;

    let hosted = storage['hosted'];
    if (typeof hosted !== typeof []) {
      // if you try to switch to Set checkout .concat usage.
      // concat compiles but doesn't work as expected
      hosted = new Array<number>();
      storage['hosted'] = hosted;
    }

    if (!(hosted as Array<unknown>).includes(target)) {
      (hosted as Array<unknown>).push(target);
    }
  }

  if (msgType === 'hostStop') {
    const msg = msgAny as HostStopMessage;
    const target = msg.target;
    printConsole('hostStop', target.toString(16));

    const hosted = storage['hosted'] as Array<number>;
    if (typeof hosted === typeof []) {
      storage['hosted'] = hosted.filter((x) => x !== target);
    }
  }

  if (f && typeof f === 'function') handler[msgType](msgAny);
};

printConsole('Hello Multiplayer!');
printConsole('settings:', settings['skymp5-client']);

const targetIp = settings['skymp5-client']['server-ip'] as string;
const targetPort = settings['skymp5-client']['server-port'] as number;

export const getServerIp = () => {
  return targetIp;
};

export const getServerUiPort = () => {
  return targetPort === 7777 ? 3000 : (targetPort as number) + 1;
};

export const connectWhenICallAndNotWhenIImport = (): void => {
  if (storage.targetIp !== targetIp || storage.targetPort !== targetPort) {
    storage.targetIp = targetIp;
    storage.targetPort = targetPort;

    printConsole(`Connecting to ${storage.targetIp}:${storage.targetPort}`);
    networking.connect(targetIp, targetPort);
  } else {
    printConsole('Reconnect is not required');
  }
};

export class SkympClient {
  constructor(private sp: Sp, private controller: CombinedController) {
    controller.registerListenerForLookup("SkympClient", this);

    const authGameData = storage[AuthGameData.storageKey] as AuthGameData | undefined;
    if (!(authGameData?.local || authGameData?.remote)) {
      authSystem.addAuthListener((data) => {
        if (data.remote) {
          browser.setAuthData(data.remote);
        }
        storage[AuthGameData.storageKey] = data;
        this.sp.browser.setFocused(false);
        this.startClient();
      });

      authSystem.main(settings["skymp5-client"]["lobbyLocation"] as Transform);
    } else {
      this.startClient();
    }
  }

  private startClient() {
    // TODO: subscribe to events in constructor, not here
    netInfo.start();
    animDebugSystem.init(settings["skymp5-client"]["animDebug"] as animDebugSystem.AnimDebugSettings);

    playerCombatSystem.start();
    once("update", () => authSystem.setPlayerAuthMode(false));
    connectWhenICallAndNotWhenIImport();
    this.ctor();

    once("update", verifyVersion);

    let riftenUnlocked = false;
    on("update", () => {
      if (riftenUnlocked) return;
      const refr = ObjectReference.from(Game.getFormEx(0x42284));
      if (!refr) return;
      refr.lock(false, false);
      riftenUnlocked = true;
    });
  }

  private ctor() {
    // TODO: subscribe to events in constructor, not here
    this.resetView();
    this.resetRemoteServer();
    setupHooks();
    updateOwner.setup();

    sp.printConsole('SkympClient ctor');

    networking.on('connectionFailed', () => {
      printConsole('Connection failed');
    });

    networking.on('connectionDenied', (err: Record<string, any> | string) => {
      printConsole('Connection denied: ', err);
    });

    networking.on('connectionAccepted', () => {
      this.msgHandler.handleConnectionAccepted();
    });

    networking.on('disconnect', () => {
      this.msgHandler.handleDisconnect();
    });

    networking.on('message', (msgAny: Record<string, unknown> | string) => {
      netInfo.NetInfo.addReceivedPacketCount(1);
      handleMessage(
        msgAny as Record<string, unknown>,
        this.msgHandler as MsgHandler,
      );
    });

    let lastInv: Inventory | undefined;

    once('update', () => {
      const send = (msg: Record<string, unknown>) => {
        this.sendTarget.send(msg, true);
      };
      const localIdToRemoteId = (localId: number) => {
        return this.localIdToRemoteId(localId);
      };
      setUpConsoleCommands(send, localIdToRemoteId);
    });

    on('activate', (e) => {
      lastInv = getInventory(Game.getPlayer() as Actor);
      let caster = e.caster ? e.caster.getFormID() : 0;
      let target = e.target ? e.target.getFormID() : 0;

      if (!target || !caster) return;

      // Actors never have non-ff ids locally in skymp
      if (caster !== 0x14 && caster < 0xff000000) return;

      target = this.localIdToRemoteId(target);
      if (!target) return printConsole('localIdToRemoteId returned 0 (target)');

      caster = this.localIdToRemoteId(caster);
      if (!caster) return printConsole('localIdToRemoteId returned 0 (caster)');

      const openState = e.target.getOpenState();
      const enum OpenState {
        None,
        Open,
        Opening,
        Closed,
        Closing,
      }
      if (openState === OpenState.Opening || openState === OpenState.Closing)
        return;

      this.sendTarget.send(
        { t: MsgType.Activate, data: { caster, target } },
        true,
      );
      printConsole('sendActi', { caster, target });
    });

    type FurnitureId = number;
    const furnitureStreak = new Map<FurnitureId, Inventory>();

    on('containerChanged', (e) => {
      const oldContainerId = e.oldContainer ? e.oldContainer.getFormID() : 0;
      const newContainerId = e.newContainer ? e.newContainer.getFormID() : 0;
      const baseObjId = e.baseObj ? e.baseObj.getFormID() : 0;
      if (oldContainerId !== 0x14 && newContainerId !== 0x14) return;

      const furnitureRef = (Game.getPlayer() as Actor).getFurnitureReference();
      if (!furnitureRef) return;

      const furrnitureId = furnitureRef.getFormID();

      if (oldContainerId === 0x14 && newContainerId === 0) {
        let craftInputObjects = furnitureStreak.get(furrnitureId);
        if (!craftInputObjects) {
          craftInputObjects = { entries: [] };
        }
        craftInputObjects.entries.push({
          baseId: baseObjId,
          count: e.numItems,
        });
        furnitureStreak.set(furrnitureId, craftInputObjects);
        printConsole(
          `Adding ${baseObjId.toString(16)} (${e.numItems}) to recipe`,
        );
      } else if (oldContainerId === 0 && newContainerId === 0x14) {
        printConsole('Flushing recipe');
        const craftInputObjects = furnitureStreak.get(furrnitureId);
        if (craftInputObjects && craftInputObjects.entries.length) {
          furnitureStreak.delete(furrnitureId);
          const workbench = this.localIdToRemoteId(furrnitureId);
          if (!workbench) return printConsole('localIdToRemoteId returned 0');

          this.sendTarget.send(
            {
              t: MsgType.CraftItem,
              data: { workbench, craftInputObjects, resultObjectId: baseObjId },
            },
            true,
          );
          printConsole('sendCraft', {
            workbench,
            craftInputObjects,
            resultObjectId: baseObjId,
          });
        }
      }
    });

    on('containerChanged', (e) => {
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
          if (!lastInv) lastInv = getPcInventory();
          if (lastInv) {
            const newInv = getInventory(Game.getPlayer() as Actor);

            // It seems that 'ignoreWorn = false' fixes this:
            // https://github.com/skyrim-multiplayer/issue-tracker/issues/43
            // For some reason excess diff is produced when 'ignoreWorn = true'
            // I thought that it would be vice versa but that's how it works
            const ignoreWorn = false;
            const diff = getDiff(lastInv, newInv, ignoreWorn);

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
            msgs.forEach((msg) => this.sendTarget.send(msg, true));

            // Prevent emitting 1,2,3,4,5 changes when taking/putting 5 potions one by one
            // This code makes it 1,1,1,1,1 but works only for extra-less items
            // At the moment of writing this I think it's not needed for items with extras
            diff.entries.forEach((entry) => {
              if (lastInv && !hasExtras(entry)) {
                const put = entry.count > 0;
                const take = entry.count < 0;
                if (put) {
                  lastInv = removeSimpleItemsAsManyAsPossible(
                    lastInv,
                    entry.baseId,
                    entry.count,
                  );
                } else if (take) {
                  const add = { entries: [entry] };
                  add.entries[0].count *= -1;
                  lastInv = sumInventories(lastInv, add);
                }
              }
            });
          }
        }
      }
    });

    on('containerChanged', (e) => {
      const pl = Game.getPlayer() as Actor;
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
        const refrId = Game.findClosestReferenceOfType(
          e.baseObj,
          pl.getPositionX(),
          pl.getPositionY(),
          pl.getPositionZ(),
          radius,
        )?.getFormID();
        if (refrId) {
          const refr = ObjectReference.from(Game.getFormEx(refrId));
          if (refr) {
            refr.delete().then(() => {
              const t = MsgType.DropItem;
              const count = 1;
              this.sendTarget.send({ t, baseId, count }, true);
            });
          }
        }
      }
    });

    once('update', () => {
      const player = Game.getPlayer();
      if (player) {
        deathSystem.makeActorImmortal(player);
      }
    });

    on('hit', (e) => {
      const playerFormId = 0x14;
      if (e.target.getFormID() === playerFormId) return;
      if (e.aggressor.getFormID() !== playerFormId) return;
      if (sp.Weapon.from(e.source) && sp.Actor.from(e.target)) {
        this.sendTarget.send({ t: MsgType.OnHit, data: getHitData(e) }, true);
      }
    });
  }

  private resetRemoteServer() {
    const prevRemoteServer: RemoteServer = storage.remoteServer as RemoteServer;
    let rs: RemoteServer;

    if (prevRemoteServer && (prevRemoteServer.getWorldModel as unknown)) {
      rs = prevRemoteServer;
      printConsole('Restore previous RemoteServer');

      // Keep previous RemoteServer, but update func implementations
      const newObj: Record<string, unknown> =
        new RemoteServer() as unknown as Record<string, unknown>;
      const rsAny: Record<string, unknown> = rs as unknown as Record<
        string,
        unknown
      >;
      for (const key in newObj) {
        if (typeof newObj[key] === 'function') rsAny[key] = newObj[key];
      }
    } else {
      rs = new RemoteServer();
      printConsole('Creating RemoteServer');
    }

    this.sendTarget = rs;
    this.msgHandler = rs;
    this.modelSource = rs;
    storage.remoteServer = rs;
  }

  private resetView() {
    const prevView: WorldView = storage.view as WorldView;
    const view = new WorldView();
    once('update', () => {
      if (prevView && prevView.destroy) {
        prevView.destroy();
        printConsole('Previous View destroyed');
      }
      storage.view = view;
    });
    on('update', () => {
      const singlePlayerService = this.controller.lookupListener("SinglePlayerService") as SinglePlayerService;
      if (!singlePlayerService.isSinglePlayer)
        view.update((this.modelSource as ModelSource).getWorldModel());
    });
  }

  private localIdToRemoteId(localFormId: number): number {
    return localIdToRemoteId(localFormId);
  }

  public getModelSource() {
    return this.modelSource;
  }

  public getSendTarget() {
    return this.sendTarget;
  }

  private msgHandler: MsgHandler = undefined as unknown as MsgHandler;
  private modelSource?: ModelSource;
  private sendTarget: SendTarget = undefined as unknown as SendTarget;
}
