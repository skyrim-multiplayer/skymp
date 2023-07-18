import * as sp from 'skyrimPlatform';
import { Actor, Form } from 'skyrimPlatform';
import {
  Armor,
  Cell,
  Game,
  ObjectReference,
  Spell,
  TESModPlatform,
  Ui,
  Utility,
  WorldSpace,
  browser,
  on,
  once,
  printConsole,
  storage,
} from 'skyrimPlatform';

import * as netInfo from '../debug/netInfoSystem';
import * as loadGameManager from '../features/loadGameManager';
import * as updateOwner from '../gamemodeApi/updateOwner';
import * as messages from '../messages';

/* eslint-disable @typescript-eslint/no-empty-function */
import * as networking from '../networking';
import * as spSnippet from '../spSnippet';
import { ObjectReferenceEx } from '../extensions/objectReferenceEx';
import { AuthGameData } from '../features/authModel';
import { IdManager } from '../lib/idManager';
import { nameof } from '../lib/nameof';
import { setActorValuePercentage } from '../sync/actorvalues';
import { applyAppearanceToPlayer } from '../sync/appearance';
import {
  applyDeathState,
  safeRemoveRagdollFromWorld,
} from '../sync/deathSystem';
import { isBadMenuShown } from '../sync/equipment';
import { Inventory, applyInventory } from '../sync/inventory';
import { Movement } from '../sync/movement';
import { learnSpells, removeAllSpells } from '../sync/spell';
import { ModelApplyUtils } from '../view/modelApplyUtils';
import {
  getViewFromStorage,
  localIdToRemoteId,
  remoteIdToLocalId,
} from '../view/worldViewMisc';
import { FormModel, WorldModel } from './model';
import { ModelSource } from './modelSource';
import { MsgHandler } from './msgHandler';
import { SendTarget } from './sendTarget';

const onceLoad = (
  refrId: number,
  callback: (refr: ObjectReference) => void,
  maxAttempts: number = 120,
) => {
  once('update', () => {
    const refr = ObjectReference.from(Game.getFormEx(refrId));
    if (refr) {
      callback(refr);
    } else {
      maxAttempts--;
      if (maxAttempts > 0) {
        once('update', () => onceLoad(refrId, callback, maxAttempts));
      } else {
        printConsole('Failed to load object reference ' + refrId.toString(16));
      }
    }
  });
};

const skipFormViewCreation = (
  msg: messages.UpdatePropertyMessage | messages.CreateActorMessage,
) => {
  // Optimization added in #1186, however it doesn't work for doors for some reason
  return msg.refrId && msg.refrId < 0xff000000 && msg.baseRecordType !== 'DOOR';
};

//
// eventSource system
//

const setupEventSource = (ctx: any) => {
  once('update', () => {
    try {
      ctx._fn(ctx);
      printConsole(`'eventSources.${ctx._eventName}' - Added`);
    } catch (e) {
      printConsole(`'eventSources.${ctx._eventName}' -`, e);
    }
  });
};

// Handle hot reload for eventSoucres
if (Array.isArray(storage['eventSourceContexts'])) {
  storage['eventSourceContexts'] = storage['eventSourceContexts'].filter(
    (ctx: Record<string, unknown>) => !ctx._expired,
  );
  (storage['eventSourceContexts'] as any).forEach((ctx: any) => {
    setupEventSource(ctx);
  });
}

//
//
//

const showConnectionError = () => {
  // TODO: unhardcode it or render via browser
  sp.createText(
    1920 / 2,
    1080 / 2,
    `Server connection failed. This may be caused by one of the following:
1. You are not present on the SkyMP Discord server
2. You have been banned by server admins
3. There is some technical issue. Try linking your Discord account again

If you feel that something is wrong, please contact us on Discord.`,
    [255, 255, 255, 1],
    'Tavern',
  );
};

let loggingStartMoment = 0;
on('tick', () => {
  const maxLoggingDelay = 5000;
  if (loggingStartMoment && Date.now() - loggingStartMoment > maxLoggingDelay) {
    printConsole('Logging in failed. Reconnecting.');
    showConnectionError();
    networking.reconnect();
    loggingStartMoment = 0;
  }
});

class SpawnTask {
  running = false;
}

const loginWithSkympIoCredentials = () => {
  loggingStartMoment = Date.now();
  const authData = storage[AuthGameData.storageKey] as AuthGameData | undefined;
  if (authData?.local) {
    printConsole(
      `Logging in offline mode, profileId = ${authData.local.profileId}`,
    );
    networking.send(
      {
        t: messages.MsgType.CustomPacket,
        content: {
          customPacketType: 'loginWithSkympIo',
          gameData: {
            profileId: authData.local.profileId,
          },
        },
      },
      true,
    );
    return;
  }
  if (authData?.remote) {
    printConsole('Logging in as skymp.io user');
    networking.send(
      {
        t: messages.MsgType.CustomPacket,
        content: {
          customPacketType: 'loginWithSkympIo',
          gameData: {
            session: authData.remote.session,
          },
        },
      },
      true,
    );
    return;
  }

  printConsole('Not found authentication method');
};

export const getPcInventory = (): Inventory => {
  const res = storage['pcInv'];
  if (typeof res === 'object' && (res as any)['entries']) {
    return res as unknown as Inventory;
  }
  return null as unknown as Inventory;
};

const setPcInventory = (inv: Inventory): void => {
  storage['pcInv'] = inv;
};

let pcInvLastApply = 0;
on('update', () => {
  if (isBadMenuShown()) return;
  if (Date.now() - pcInvLastApply > 5000) {
    pcInvLastApply = Date.now();
    const pcInv = getPcInventory();
    if (pcInv) applyInventory(Game.getPlayer() as Actor, pcInv, false, true);
  }
});

const unequipIronHelmet = () => {
  const ironHelment = Armor.from(Game.getFormEx(0x00012e4d));
  const pl = Game.getPlayer();
  if (pl) pl.unequipItem(ironHelment, false, true);
};

export class RemoteServer implements MsgHandler, ModelSource, SendTarget {
  setInventory(msg: messages.SetInventory): void {
    once('update', () => {
      setPcInventory(msg.inventory);
      pcInvLastApply = 0;
    });
  }

  openContainer(msg: messages.OpenContainer): void {
    once('update', async () => {
      await Utility.wait(0.1); // Give a chance to update inventory
      (
        ObjectReference.from(Game.getFormEx(msg.target)) as ObjectReference
      ).activate(Game.getPlayer(), true);
      (async () => {
        while (!Ui.isMenuOpen('ContainerMenu')) await Utility.wait(0.1);
        while (Ui.isMenuOpen('ContainerMenu')) await Utility.wait(0.1);
        networking.send(
          {
            t: messages.MsgType.Activate,
            data: { caster: 0x14, target: msg.target },
          },
          true,
        );
      })();
    });
  }

  teleport(msg: messages.Teleport): void {
    once('update', () => {
      printConsole(
        'Teleporting...',
        msg.pos,
        'cell/world is',
        msg.worldOrCell.toString(16),
      );
      // todo: think about track ragdoll state of player
      safeRemoveRagdollFromWorld(Game.getPlayer()!, () => {
        TESModPlatform.moveRefrToPosition(
          Game.getPlayer()!,
          Cell.from(Game.getFormEx(msg.worldOrCell)),
          WorldSpace.from(Game.getFormEx(msg.worldOrCell)),
          msg.pos[0],
          msg.pos[1],
          msg.pos[2],
          msg.rot[0],
          msg.rot[1],
          msg.rot[2],
        );
      });
    });
  }

  createActor(msg: messages.CreateActorMessage): void {
    if (skipFormViewCreation(msg)) {
      const refrId = msg.refrId!;
      onceLoad(refrId, (refr: ObjectReference) => {
        if (refr) {
          ObjectReferenceEx.dealWithRef(refr, refr.getBaseObject() as Form);
          if (msg.inventory) {
            ModelApplyUtils.applyModelInventory(refr, msg.inventory);
          }
          if (msg.props) {
            ModelApplyUtils.applyModelIsOpen(refr, !!msg.props['isOpen']);
            ModelApplyUtils.applyModelIsHarvested(
              refr,
              !!msg.props['isHarvested'],
            );
          }
        } else {
          printConsole('Failed to apply model to', refrId.toString(16));
        }
      });
      return;
    }

    loggingStartMoment = 0;

    const i = this.getIdManager().allocateIdFor(msg.idx);
    if (this.worldModel.forms.length <= i) this.worldModel.forms.length = i + 1;

    let movement: Movement = null as unknown as Movement;
    if ((msg.refrId as number) >= 0xff000000) {
      movement = {
        pos: msg.transform.pos,
        rot: msg.transform.rot,
        worldOrCell: msg.transform.worldOrCell,
        runMode: 'Standing',
        direction: 0,
        isInJumpState: false,
        isSneaking: false,
        isBlocking: false,
        isWeapDrawn: false,
        isDead: false,
        healthPercentage: 1.0,
        speed: 0,
      };
    }

    this.worldModel.forms[i] = {
      idx: msg.idx,
      movement,
      numMovementChanges: 0,
      numAppearanceChanges: 0,
      baseId: msg.baseId,
      refrId: msg.refrId,
      isMyClone: msg.isMe,
    };
    if (msg.isMe) {
      updateOwner.setOwnerModel(this.worldModel.forms[i]);
    }

    if (msg.appearance) {
      this.worldModel.forms[i].appearance = msg.appearance;
    }

    if (msg.equipment) {
      this.worldModel.forms[i].equipment = msg.equipment;
    }

    if (msg.props) {
      for (const propName in msg.props) {
        const i = this.getIdManager().getId(msg.idx);
        (this.worldModel.forms[i] as Record<string, unknown>)[propName] =
          msg.props[propName];
      }
    }

    if (msg.isMe) this.worldModel.playerCharacterFormIdx = i;

    // TODO: move to a separate module

    if (msg.props && !msg.props.isHostedByOther) {
    }

    if (msg.props && msg.props.isRaceMenuOpen && msg.isMe)
      this.setRaceMenuOpen({ type: 'setRaceMenuOpen', open: true });

    const applyPcInv = () => {
      applyInventory(
        Game.getPlayer() as Actor,
        msg.equipment
          ? {
              entries: msg.equipment.inv.entries.filter(
                (x) => !!Armor.from(Game.getFormEx(x.baseId)),
              ),
            }
          : { entries: [] },
        false,
      );
      if (msg.props && msg.props.inventory)
        this.setInventory({
          type: 'setInventory',
          inventory: (msg.props as any).inventory as Inventory,
        });
    };

    if (msg.isMe && msg.props) {
      const learnedSpells = msg.props['learnedSpells'] as Array<number>;

      once('update', () => {
        Utility.wait(1).then(() => {
          const player = Game.getPlayer();

          if (player) {
            removeAllSpells(player);
            learnSpells(player, learnedSpells);
            printConsole(
              `player learnedSpells: ${JSON.stringify(learnedSpells)}`,
            );
          }
        });
      });
    }

    if (msg.isMe) {
      const task = new SpawnTask();
      once('update', () => {
        if (!task.running) {
          task.running = true;
          printConsole('Using moveRefrToPosition to spawn player');
          (async () => {
            while (true) {
              printConsole('Spawning...');
              TESModPlatform.moveRefrToPosition(
                Game.getPlayer(),
                Cell.from(Game.getFormEx(msg.transform.worldOrCell)),
                WorldSpace.from(Game.getFormEx(msg.transform.worldOrCell)),
                msg.transform.pos[0],
                msg.transform.pos[1],
                msg.transform.pos[2],
                msg.transform.rot[0],
                msg.transform.rot[1],
                msg.transform.rot[2],
              );
              await Utility.wait(1);
              const pl = Game.getPlayer();
              if (!pl) break;
              const pos = [
                pl.getPositionX(),
                pl.getPositionY(),
                pl.getPositionZ(),
              ];
              const sqr = (x: number) => x * x;
              const distance = Math.sqrt(
                sqr(pos[0] - msg.transform.pos[0]) +
                  sqr(pos[1] - msg.transform.pos[1]),
              );
              if (distance < 256) {
                break;
              }
            }
          })();
          // Unfortunatelly it requires two calls to work
          Utility.wait(1).then(applyPcInv);
          Utility.wait(1.3).then(applyPcInv);
          // Note: appearance part was copy-pasted
          if (msg.appearance) {
            applyAppearanceToPlayer(msg.appearance);
            if (msg.appearance.isFemale)
              // Fix gender-specific walking anim
              (Game.getPlayer() as Actor).resurrect();
          }
        }

        if (msg.props) {
          const baseActorValues = new Map<string, unknown>([
            ['healRate', msg.props.healRate],
            ['healRateMult', msg.props.healRateMult],
            ['health', msg.props.health],
            ['magickaRate', msg.props.magickaRate],
            ['magickaRateMult', msg.props.magickaRateMult],
            ['magicka', msg.props.magicka],
            ['staminaRate', msg.props.staminaRate],
            ['staminaRateMult', msg.props.staminaRateMult],
            ['stamina', msg.props.stamina],
            ['healthPercentage', msg.props.healthPercentage],
            ['staminaPercentage', msg.props.staminaPercentage],
            ['magickaPercentage', msg.props.magickaPercentage],
          ]);

          const player = Game.getPlayer();
          if (player) {
            baseActorValues.forEach((value, key) => {
              if (typeof value === 'number') {
                if (key.includes('Percentage')) {
                  const subKey = key.replace('Percentage', '');
                  const subValue = baseActorValues.get(subKey);
                  if (typeof subValue === 'number') {
                    setActorValuePercentage(player, subKey, value);
                  }
                } else {
                  player.setActorValue(key, value);
                }
              }
            });
          }
        }
      });
      once('tick', () => {
        once('tick', () => {
          if (!task.running) {
            task.running = true;
            printConsole('Using loadGame to spawn player');
            printConsole(
              'skinColorFromServer:',
              msg.appearance
                ? msg.appearance.skinColor.toString(16)
                : undefined,
            );
            loadGameManager.loadGame(
              msg.transform.pos,
              msg.transform.rot,
              msg.transform.worldOrCell,
              msg.appearance
                ? {
                    name: msg.appearance.name,
                    raceId: msg.appearance.raceId,
                    face: {
                      hairColor: msg.appearance.hairColor,
                      bodySkinColor: msg.appearance.skinColor,
                      headTextureSetId: msg.appearance.headTextureSetId,
                      headPartIds: msg.appearance.headpartIds,
                      presets: msg.appearance.presets,
                    },
                  }
                : undefined,
            );
            once('update', () => {
              applyPcInv();
              Utility.wait(0.3).then(applyPcInv);
              // Note: appearance part was copy-pasted
              if (msg.appearance) {
                applyAppearanceToPlayer(msg.appearance);
                if (msg.appearance.isFemale)
                  // Fix gender-specific walking anim
                  (Game.getPlayer() as Actor).resurrect();
              }
            });
          }
        });
      });
    }
  }

  destroyActor(msg: messages.DestroyActorMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i] = null as unknown as FormModel;
    getViewFromStorage()?.syncFormArray(this.worldModel);

    // Shrink to fit
    while (1) {
      const length = this.worldModel.forms.length;
      if (!length) break;
      if (this.worldModel.forms[length - 1]) break;
      this.worldModel.forms.length = length - 1;
    }

    if (this.worldModel.playerCharacterFormIdx === i) {
      this.worldModel.playerCharacterFormIdx = -1;

      // TODO: move to a separate module
      once('update', () => Game.quitToMainMenu());
    }

    this.getIdManager().freeIdFor(msg.idx);
  }

  UpdateMovement(msg: messages.UpdateMovementMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].movement = msg.data;
    if (!this.worldModel.forms[i].numMovementChanges) {
      this.worldModel.forms[i].numMovementChanges = 0;
    }
    (this.worldModel.forms[i].numMovementChanges as number)++;
  }

  UpdateAnimation(msg: messages.UpdateAnimationMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].animation = msg.data;
  }

  UpdateAppearance(msg: messages.UpdateAppearanceMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].appearance = msg.data;
    if (!this.worldModel.forms[i].numAppearanceChanges) {
      this.worldModel.forms[i].numAppearanceChanges = 0;
    }
    (this.worldModel.forms[i].numAppearanceChanges as number)++;
  }

  UpdateEquipment(msg: messages.UpdateEquipmentMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].equipment = msg.data;
  }

  UpdateProperty(msg: messages.UpdatePropertyMessage): void {
    if (skipFormViewCreation(msg)) {
      const refrId = msg.refrId;
      once('update', () => {
        const refr = ObjectReference.from(Game.getFormEx(refrId));
        if (!refr) {
          printConsole('UpdateProperty: refr not found');
          return;
        }
        if (msg.propName === 'inventory') {
          ModelApplyUtils.applyModelInventory(refr, msg.data as Inventory);
        } else if (msg.propName === 'isOpen') {
          ModelApplyUtils.applyModelIsOpen(refr, !!msg.data);
        } else if (msg.propName === 'isHarvested') {
          ModelApplyUtils.applyModelIsHarvested(refr, !!msg.data);
        }
      });
      return;
    }
    const i = this.getIdManager().getId(msg.idx);
    const form = this.worldModel.forms[i];
    (form as Record<string, unknown>)[msg.propName] = msg.data;
  }

  DeathStateContainer(msg: messages.DeathStateContainerMessage): void {
    once('update', () =>
      printConsole(`Received death state: ${JSON.stringify(msg.tIsDead)}`),
    );
    if (
      msg.tIsDead.propName !== nameof<FormModel>('isDead') ||
      typeof msg.tIsDead.data !== 'boolean'
    )
      return;

    if (msg.tChangeValues) {
      this.ChangeValues(msg.tChangeValues);
    }
    once('update', () => this.UpdateProperty(msg.tIsDead));

    if (msg.tTeleport) {
      this.teleport(msg.tTeleport);
    }

    const id = this.getIdManager().getId(msg.tIsDead.idx);
    const form = this.worldModel.forms[id];
    once('update', () => {
      const actor =
        id === this.getWorldModel().playerCharacterFormIdx
          ? Game.getPlayer()!
          : Actor.from(Game.getFormEx(remoteIdToLocalId(form.refrId ?? 0)));
      if (actor) {
        applyDeathState(actor, msg.tIsDead.data as boolean);
      }
    });
  }

  handleConnectionAccepted(): void {
    this.worldModel.forms = [];
    this.worldModel.playerCharacterFormIdx = -1;

    loginWithSkympIoCredentials();
  }

  handleDisconnect(): void {}

  ChangeValues(msg: messages.ChangeValuesMessage): void {
    once('update', () => {
      const ac = Game.getPlayer();
      if (!ac) return;
      setActorValuePercentage(ac, 'health', msg.data.health);
      setActorValuePercentage(ac, 'stamina', msg.data.stamina);
      setActorValuePercentage(ac, 'magicka', msg.data.magicka);
    });
  }

  setRaceMenuOpen(msg: messages.SetRaceMenuOpenMessage): void {
    if (msg.open) {
      // wait 0.3s cause we can see visual bugs when teleporting
      // and showing this menu at the same time in onConnect
      once('update', () =>
        Utility.wait(0.3).then(() => {
          unequipIronHelmet();
          Game.showRaceMenu();
        }),
      );
    } else {
      // TODO: Implement closeMenu in SkyrimPlatform
    }
  }

  customPacket(msg: messages.CustomPacket): void {
    switch (msg.content.customPacketType) {
      case 'loginRequired':
        loginWithSkympIoCredentials();
        break;
    }
  }

  spSnippet(msg: messages.SpSnippet): void {
    once('update', async () => {
      spSnippet
        .run(msg)
        .then((res) => {
          if (res === undefined) res = null;
          this.send(
            {
              t: messages.MsgType.FinishSpSnippet,
              returnValue: res,
              snippetIdx: msg.snippetIdx,
            },
            true,
          );
        })
        .catch((e) => printConsole('!!! SpSnippet failed', e));
    });
  }

  private updateGamemodeUpdateFunctions(
    storageVar: string,
    functionSources: Record<string, string>,
  ): void {
    storage[storageVar] = JSON.parse(JSON.stringify(functionSources));
    for (const propName of Object.keys(functionSources)) {
      try {
        (storage[storageVar] as any)[propName] = new Function(
          'ctx',
          (storage[storageVar] as any)[propName],
        );
        const emptyFunction = functionSources[propName] === '';
        if (emptyFunction) {
          delete (storage[storageVar] as any)[propName];
          printConsole(`'${storageVar}.${propName}' -`, 'Added empty');
        } else {
          printConsole(`'${storageVar}.${propName}' -`, 'Added');
        }
      } catch (e) {
        printConsole(`'${storageVar}.${propName}' -`, e);
      }
    }
    storage[`${storageVar}_keys`] = Object.keys(storage[storageVar] as any);
  }

  updateGamemodeData(msg: messages.UpdateGamemodeDataMessage): void {
    //
    // updateOwnerFunctions/updateNeighborFunctions
    //
    storage['updateNeighborFunctions'] = undefined;
    storage['updateOwnerFunctions'] = undefined;

    this.updateGamemodeUpdateFunctions(
      'updateNeighborFunctions',
      msg.updateNeighborFunctions || {},
    );
    this.updateGamemodeUpdateFunctions(
      'updateOwnerFunctions',
      msg.updateOwnerFunctions || {},
    );

    //
    // EventSource
    //
    if (!Array.isArray(storage['eventSourceContexts'])) {
      storage['eventSourceContexts'] = [];
    } else {
      storage['eventSourceContexts'].forEach((ctx: Record<string, unknown>) => {
        ctx.sendEvent = () => {};
        ctx._expired = true;
      });
    }
    const eventNames = Object.keys(msg.eventSources);
    eventNames.forEach((eventName) => {
      try {
        const fn = new Function('ctx', msg.eventSources[eventName]);
        const ctx = {
          sp,
          sendEvent: (...args: unknown[]) => {
            this.send(
              {
                t: messages.MsgType.CustomEvent,
                args,
                eventName,
              },
              true,
            );
          },
          getFormIdInServerFormat: (clientsideFormId: number) => {
            return localIdToRemoteId(clientsideFormId);
          },
          getFormIdInClientFormat: (serversideFormId: number) => {
            return remoteIdToLocalId(serversideFormId);
          },
          _fn: fn,
          _eventName: eventName,
          state: {},
        };
        (storage['eventSourceContexts'] as Record<string, any>).push(ctx);
        setupEventSource(ctx);
      } catch (e) {
        printConsole(`'eventSources.${eventName}' -`, e);
      }
    });
  }

  /** Packet handlers end **/

  getWorldModel(): WorldModel {
    return this.worldModel;
  }

  getMyActorIndex(): number {
    return this.worldModel.playerCharacterFormIdx;
  }

  send(msg: Record<string, unknown>, reliable: boolean): void {
    if (this.worldModel.playerCharacterFormIdx === -1) return;

    const refrId = msg._refrId as number | undefined;

    const idxInModel = refrId
      ? this.worldModel.forms.findIndex((f) => f && f.refrId === refrId)
      : this.worldModel.playerCharacterFormIdx;
    // fixes "can't get property idx of null or undefined"
    if (!this.worldModel.forms[idxInModel]) return;
    msg.idx = this.worldModel.forms[idxInModel].idx;

    delete msg._refrId;
    netInfo.NetInfo.addSentPacketCount(1);
    networking.send(msg, reliable);
  }

  private getIdManager() {
    if (!this.idManager_) this.idManager_ = new IdManager();
    return this.idManager_;
  }

  private worldModel: WorldModel = { forms: [], playerCharacterFormIdx: -1 };
  private idManager_ = new IdManager();
}
