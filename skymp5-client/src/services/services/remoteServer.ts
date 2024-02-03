import * as sp from 'skyrimPlatform';
import { Actor, Form } from 'skyrimPlatform';
import {
  Armor,
  Cell,
  Game,
  ObjectReference,
  TESModPlatform,
  Ui,
  Utility,
  WorldSpace,
  on,
  once,
  printConsole,
  storage,
} from 'skyrimPlatform';

import * as updateOwner from '../../gamemodeApi/updateOwner';
import * as messages from '../../messages';

/* eslint-disable @typescript-eslint/no-empty-function */
import { ObjectReferenceEx } from '../../extensions/objectReferenceEx';
import { AuthGameData } from '../../features/authModel';
import { IdManager } from '../../lib/idManager';
import { nameof } from '../../lib/nameof';
import { setActorValuePercentage } from '../../sync/actorvalues';
import { applyAppearanceToPlayer } from '../../sync/appearance';
import { isBadMenuShown } from '../../sync/equipment';
import { Inventory, applyInventory } from '../../sync/inventory';
import { Movement } from '../../sync/movement';
import { learnSpells, removeAllSpells } from '../../sync/spell';
import { ModelApplyUtils } from '../../view/modelApplyUtils';
import { FormModel, WorldModel } from '../../view/model';
import { LoadGameService } from './loadGameService';
import { UpdateMovementMessage } from '../messages/updateMovementMessage';
import { ChangeValuesMessage } from '../messages/changeValues';
import { UpdateAnimationMessage } from '../messages/updateAnimationMessage';
import { UpdateEquipmentMessage } from '../messages/updateEquipmentMessage';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { CustomEventMessage } from '../messages/customEventMessage';
import { RagdollService } from './ragdollService';
import { UpdateAppearanceMessage } from '../messages/updateAppearanceMessage';
import { TeleportMessage } from '../messages/teleportMessage';
import { DeathStateContainerMessage } from '../messages/deathStateContainerMessage';
import { RespawnNeededError } from '../../lib/errors';
import { OpenContainerMessage } from '../messages/openContainerMessage';
import { NetworkingService } from './networkingService';
import { ActivateMessage } from '../messages/activateMessage';
import { ClientListener, CombinedController, Sp } from './clientListener';
import { HostStartMessage } from '../messages/hostStartMessage';
import { HostStopMessage } from '../messages/hostStopMessage';
import { ConnectionMessage } from '../events/connectionMessage';
import { SetInventoryMessage } from '../messages/setInventoryMessage';
import { CreateActorMessage } from '../messages/createActorMessage';
import { UpdateGamemodeDataMessage } from '../messages/updateGameModeDataMessage';
import { CustomPacketMessage2 } from '../messages/customPacketMessage2';
import { DestroyActorMessage } from '../messages/destroyActorMessage';
import { SetRaceMenuOpenMessage } from '../messages/setRaceMenuOpenMessage';
import { UpdatePropertyMessage } from '../messages/updatePropertyMessage';
import { TeleportMessage2 } from '../messages/teleportMessage2';

// TODO: refactor worldViewMisc into service
import {
  getObjectReference,
  getViewFromStorage,
  localIdToRemoteId,
  remoteIdToLocalId,
} from '../../view/worldViewMisc';
import { TimeService } from './timeService';

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
  msg: UpdatePropertyMessage | CreateActorMessage,
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
  printConsole("Server connection failed. This may be caused by one of the following:");
  printConsole("1. You are not present on the SkyMP Discord server");
  printConsole("2. You have been banned by server admins");
  printConsole("3. There is some technical issue. Try linking your Discord account again");
  printConsole("If you feel that something is wrong, please contact us on Discord.");
};

export const getPcInventory = (): Inventory | undefined => {
  const res = storage['pcInv'];
  if (typeof res === 'object' && (res as any)['entries']) {
    return res as Inventory;
  }
  return undefined;
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

export class RemoteServer extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    this.controller.on("tick", () => this.onTick());

    this.controller.emitter.on("hostStartMessage", (e) => this.onHostStartMessage(e));
    this.controller.emitter.on("hostStopMessage", (e) => this.onHostStopMessage(e));
    this.controller.emitter.on("setInventoryMessage", (e) => this.onSetInventoryMessage(e));
    this.controller.emitter.on("openContainerMessage", (e) => this.onOpenContainerMessage(e));
    this.controller.emitter.on("updateMovementMessage", (e) => this.onUpdateMovementMessage(e));
    this.controller.emitter.on("updateAnimationMessage", (e) => this.onUpdateAnimationMessage(e));
    this.controller.emitter.on("updateEquipmentMessage", (e) => this.onUpdateEquipmentMessage(e));
    this.controller.emitter.on("changeValuesMessage", (e) => this.onChangeValuesMessage(e));
    this.controller.emitter.on("updateAppearanceMessage", (e) => this.onUpdateAppearanceMessage(e));
    this.controller.emitter.on("teleportMessage", (e) => this.onTeleportMessage(e));
    this.controller.emitter.on("teleportMessage2", (e) => this.onTeleportMessage(e));
    this.controller.emitter.on("createActorMessage", (e) => this.onCreateActorMessage(e));
    this.controller.emitter.on("customPacketMessage2", (e) => this.onCustomPacketMessage2(e));
    this.controller.emitter.on("destroyActorMessage", (e) => this.onDestroyActorMessage(e));
    this.controller.emitter.on("setRaceMenuOpenMessage", (e) => this.onSetRaceMenuOpenMessage(e));
    this.controller.emitter.on("updateGamemodeDataMessage", (e) => this.onUpdateGamemodeDataMessage(e));
    this.controller.emitter.on("updatePropertyMessage", (e) => this.onUpdatePropertyMessage(e));
    this.controller.emitter.on("deathStateContainerMessage", (e) => this.onDeathStateContainerMessage(e));

    this.controller.emitter.on("connectionAccepted", () => this.handleConnectionAccepted());
  }

  private onTick() {
    // TODO: Should be no hardcoded/magic-number limit
    // TODO: Busy waiting is bad. Should be replaced with some kind of event
    const maxLoggingDelay = 15000;
    if (this.loggingStartMoment && Date.now() - this.loggingStartMoment > maxLoggingDelay) {
      printConsole('Logging in failed. Reconnecting.');
      showConnectionError();
      this.controller.lookupListener(NetworkingService).reconnect();
      this.loggingStartMoment = 0;
    }
  }

  private onHostStartMessage(event: ConnectionMessage<HostStartMessage>) {
    const msg = event.message;
    const target = msg.target;

    let hosted = storage['hosted'];
    if (typeof hosted !== typeof []) {
      // if you try to switch to Set please checkout .concat usage.
      // concat compiles but doesn't work as expected
      hosted = new Array<number>();
      storage['hosted'] = hosted;
    }

    if (!(hosted as Array<unknown>).includes(target)) {
      (hosted as Array<unknown>).push(target);
    }
  }

  private onHostStopMessage(event: ConnectionMessage<HostStopMessage>) {
    const msg = event.message;
    const target = msg.target;
    this.logTrace('hostStop ' + target.toString(16));

    const hosted = storage['hosted'] as Array<number>;
    if (typeof hosted === typeof []) {
      storage['hosted'] = hosted.filter((x) => x !== target);
    }
  }

  private onSetInventoryMessage(event: ConnectionMessage<SetInventoryMessage>): void {
    const msg = event.message;
    once('update', () => {
      setPcInventory(msg.inventory);
      pcInvLastApply = 0;
    });
  }

  private onOpenContainerMessage(event: ConnectionMessage<OpenContainerMessage>): void {
    once('update', async () => {
      await Utility.wait(0.1); // Give a chance to update inventory
      (
        ObjectReference.from(Game.getFormEx(event.message.target)) as ObjectReference
      ).activate(Game.getPlayer(), true);
      (async () => {
        while (!Ui.isMenuOpen('ContainerMenu')) await Utility.wait(0.1);
        while (Ui.isMenuOpen('ContainerMenu')) await Utility.wait(0.1);

        const message: ActivateMessage = {
          t: messages.MsgType.Activate,
          data: {
            caster: 0x14, target: event.message.target
          }
        };

        this.controller.emitter.emit("sendMessage", {
          message: message,
          reliability: "reliable"
        });
      })();
    });
  }

  private onTeleportMessage(event: ConnectionMessage<TeleportMessage> | ConnectionMessage<TeleportMessage2>): void {
    const msg = event.message;
    once('update', () => {
      const id = ("idx" in msg && typeof msg.idx === "number") ? this.getIdManager().getId(msg.idx) : this.getMyActorIndex();
      const refr = id === this.getMyActorIndex() ? Game.getPlayer() : getObjectReference(id);
      printConsole(
        `Teleporting (id=${id}) ${refr?.getFormID().toString(16)}...`,
        msg.pos,
        'cell/world is',
        msg.worldOrCell.toString(16),
      );
      const ragdollService = this.controller.lookupListener(RagdollService);

      const refrId = refr?.getFormID();

      const removeRagdollCallback = () => {
        TESModPlatform.moveRefrToPosition(
          ObjectReference.from(Game.getFormEx(refrId || 0)),
          Cell.from(Game.getFormEx(msg.worldOrCell)),
          WorldSpace.from(Game.getFormEx(msg.worldOrCell)),
          msg.pos[0],
          msg.pos[1],
          msg.pos[2],
          msg.rot[0],
          msg.rot[1],
          msg.rot[2],
        );
      };
      const actor = Actor.from(refr);
      if (actor /*&& actor.getFormID() === 0x14*/) {
        ragdollService.safeRemoveRagdollFromWorld(actor, removeRagdollCallback);
      }
      else {
        removeRagdollCallback();
      }
    });
  }

  private onCreateActorMessage(event: ConnectionMessage<CreateActorMessage>): void {
    const msg = event.message;
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

            // TODO: move to a separate module
            if (msg.props.setNodeTextureSet) {
              const setNodeTextureSet = msg.props.setNodeTextureSet as Record<string, number>;
              for (const key in setNodeTextureSet) {
                const textureSetId = setNodeTextureSet[key];
                const firstPerson = false;

                const textureSet = this.sp.TextureSet.from(Game.getFormEx(textureSetId));
                if (textureSet !== null) {
                  sp.NetImmerse.setNodeTextureSet(refr, key, textureSet, firstPerson);
                  this.logTrace(`Applied texture set ${textureSetId.toString(16)} to ${key}`);
                } else {
                  this.logError(`Failed to apply texture set ${textureSetId.toString(16)} to ${key}`);
                }
              }
            }

            ModelApplyUtils.applyModelIsDisabled(refr, !!msg.props['disabled']);

            // TODO: move to a separate module
            const animation = msg.props.lastAnimation;
            if (typeof animation === "string") {
              const refrid = refr.getFormID();

              (async () => {
                for (let i = 0; i < 5; i++) {
                  // retry. pillars in bleakfalls are not reliable for some reason
                  let res2 = ObjectReference.from(Game.getFormEx(refrid))?.playAnimation(animation);
                  if (res2) break;
                  await Utility.wait(2);
                }
              })();
            }


            const displayName = msg.props.displayName;
            if (typeof displayName === "string") {
              refr.setDisplayName(displayName, true);
              this.logTrace(`calling setDisplayName "${displayName}" for ${refr.getFormID().toString(16)}`);
            }
          }
        } else {
          printConsole('Failed to apply model to', refrId.toString(16));
        }
      });
      return;
    }

    printConsole("Create actor")

    this.loggingStartMoment = 0;

    const i = this.getIdManager().allocateIdFor(msg.idx);
    if (this.worldModel.forms.length <= i) this.worldModel.forms.length = i + 1;

    let movement: Movement = null as unknown as Movement;
    // TODO: better check if it is an npc (not an object reference)
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

    if (msg.isDead) {
      this.worldModel.forms[i].isDead = msg.isDead;
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
      this.onSetRaceMenuOpenMessage({ message: { type: 'setRaceMenuOpen', open: true } });

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
        this.onSetInventoryMessage({
          message: {
            type: 'setInventory',
            inventory: (msg.props as any).inventory as Inventory,
          }
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
      const spawnTask = { running: false };
      once('update', () => {
        // Use MoveRefrToPosition to spawn if possible (not in main menu)
        // In case of connection lost this is essential
        if (!spawnTask.running) {
          spawnTask.running = true;
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
          if (!spawnTask.running) {
            spawnTask.running = true;

            let loadOrder = new Array<string>();
            for (let i = 0; i < this.sp.Game.getModCount(); ++i) {
              loadOrder.push(this.sp.Game.getModName(i));
            }

            this.logTrace(`loading game in world/cell ${msg.transform.worldOrCell.toString(16)}`);
            const loadGameService = this.controller.lookupListener(LoadGameService);
            loadGameService.loadGame(
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
              loadOrder,
              { minutes: 0, seconds: 0, hours: this.controller.lookupListener(TimeService).getTime().newGameHourValue }
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

  private onDestroyActorMessage(event: ConnectionMessage<DestroyActorMessage>): void {
    const msg = event.message;

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

  private onUpdateMovementMessage(event: ConnectionMessage<UpdateMovementMessage>): void {
    const msg = event.message;

    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].movement = msg.data;
    if (!this.worldModel.forms[i].numMovementChanges) {
      this.worldModel.forms[i].numMovementChanges = 0;
    }
    (this.worldModel.forms[i].numMovementChanges as number)++;
  }

  private onUpdateAnimationMessage(event: ConnectionMessage<UpdateAnimationMessage>): void {
    const msg = event.message;

    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].animation = msg.data;
  }

  private onUpdateAppearanceMessage(event: ConnectionMessage<UpdateAppearanceMessage>): void {
    const msg = event.message;

    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].appearance = msg.data || undefined;
    if (!this.worldModel.forms[i].numAppearanceChanges) {
      this.worldModel.forms[i].numAppearanceChanges = 0;
    }
    (this.worldModel.forms[i].numAppearanceChanges as number)++;

    const newAppearance = msg.data;

    if (i === this.getMyActorIndex() && newAppearance) {
      this.controller.once("update", () => {
        applyAppearanceToPlayer(newAppearance);
        this.logTrace("Applied appearance to the player");
      });
    }
  }

  private onUpdateEquipmentMessage(event: ConnectionMessage<UpdateEquipmentMessage>): void {
    const msg = event.message;

    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].equipment = msg.data;
  }

  private onUpdatePropertyMessage(event: ConnectionMessage<UpdatePropertyMessage>): void {
    const msg = event.message;

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
        } else if (msg.propName === 'disabled') {
          ModelApplyUtils.applyModelIsDisabled(refr, !!msg.data);
        }
      });
      return;
    }
    const i = this.getIdManager().getId(msg.idx);
    const form = this.worldModel.forms[i];
    (form as Record<string, unknown>)[msg.propName] = msg.data;
  }

  private onDeathStateContainerMessage(event: ConnectionMessage<DeathStateContainerMessage>): void {
    const msg = event.message;

    once('update', () =>
      printConsole(`Received death state: ${JSON.stringify(msg.tIsDead)}`),
    );
    if (
      msg.tIsDead.propName !== nameof<FormModel>('isDead') ||
      typeof msg.tIsDead.data !== 'boolean'
    )
      return;

    if (msg.tChangeValues) {
      this.onChangeValuesMessage({ message: msg.tChangeValues });
    }
    once('update', () => this.onUpdatePropertyMessage({ message: msg.tIsDead }));

    if (msg.tTeleport) {
      this.onTeleportMessage({ message: msg.tTeleport });
    }

    const id = this.getIdManager().getId(msg.tIsDead.idx);
    const form = this.worldModel.forms[id];
    once('update', () => {
      const actor =
        id === this.getWorldModel().playerCharacterFormIdx
          ? Game.getPlayer()!
          : Actor.from(Game.getFormEx(remoteIdToLocalId(form.refrId ?? 0)));
      if (actor) {
        try {
          this.controller.emitter.emit("applyDeathStateEvent", {
            actor: actor,
            isDead: msg.tIsDead.data as boolean
          });
        } catch (e) {
          if (e instanceof RespawnNeededError) {
            actor.disableNoWait(false);
            actor.delete();
          } else {
            throw e;
          }
        }
      }
    });
  }

  private handleConnectionAccepted(): void {
    this.worldModel.forms = [];
    this.worldModel.playerCharacterFormIdx = -1;

    this.logTrace("Handle connection accepted");

    this.loginWithSkympIoCredentials();
  }

  private onChangeValuesMessage(event: ConnectionMessage<ChangeValuesMessage>): void {
    const msg = event.message;

    once('update', () => {
      const id = this.getIdManager().getId(msg.idx);
      const refr = id === this.getMyActorIndex() ? Game.getPlayer() : getObjectReference(id);
      const ac = Actor.from(refr);
      if (!ac) return;
      setActorValuePercentage(ac, 'health', msg.data.health);
      setActorValuePercentage(ac, 'stamina', msg.data.stamina);
      setActorValuePercentage(ac, 'magicka', msg.data.magicka);
    });
  }

  private onSetRaceMenuOpenMessage(event: ConnectionMessage<SetRaceMenuOpenMessage>): void {
    const msg = event.message;

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

  private onCustomPacketMessage2(event: ConnectionMessage<CustomPacketMessage2>): void {
    const msg = event.message;

    switch (msg.content.customPacketType) {
      case 'loginRequired':
        this.logTrace('loginRequired received');
        this.loginWithSkympIoCredentials();
        break;
    }
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

  private onUpdateGamemodeDataMessage(event: ConnectionMessage<UpdateGamemodeDataMessage>): void {
    const msg = event.message;

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
        ctx.sendEvent = () => { };
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
            const message: CustomEventMessage = {
              t: messages.MsgType.CustomEvent,
              args,
              eventName
            };
            this.controller.emitter.emit("sendMessage", {
              message: message,
              reliability: "reliable"
            });
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

  private getIdManager() {
    return this.idManager_;
  }

  private get worldModel(): WorldModel {
    if (typeof storage["worldModel"] === "function") {
      storage["worldModel"] = { forms: [], playerCharacterFormIdx: -1 };
    }
    return storage["worldModel"] as WorldModel;
  }

  private get idManager_(): IdManager {
    if (typeof storage["idManager"] === "function") {
      // Note: full IdManager object preserved across hot-reloads, including methods.
      storage["idManager"] = new IdManager();
    }
    return storage["idManager"] as IdManager;
  }

  private loginWithSkympIoCredentials() {
    this.loggingStartMoment = Date.now();

    const authData = storage[AuthGameData.storageKey] as AuthGameData | undefined;
    if (authData?.local) {
      this.logTrace(
        `Logging in offline mode, profileId = ${authData.local.profileId}`,
      );
      const message: CustomPacketMessage = {
        t: messages.MsgType.CustomPacket,
        content: {
          customPacketType: 'loginWithSkympIo',
          gameData: {
            profileId: authData.local.profileId,
          },
        },
      };
      this.controller.emitter.emit("sendMessage", {
        message: message,
        reliability: "reliable"
      });
      return;
    }

    if (authData?.remote) {
      this.logTrace('Logging in as a master API user');
      const message: CustomPacketMessage = {
        t: messages.MsgType.CustomPacket,
        content: {
          customPacketType: 'loginWithSkympIo',
          gameData: {
            session: authData.remote.session,
          },
        },
      };
      this.controller.emitter.emit("sendMessage", {
        message: message,
        reliability: "reliable"
      });
      return;
    }

    this.logError('Not found authentication method');
  };

  private loggingStartMoment = 0;
}
