import {
  Actor,
  ActorBase,
  Form,
  FormType,
  Game,
  Keyword,
  NetImmerse,
  ObjectReference,
  TESModPlatform,
  Utility,
  createText,
  destroyText,
  once,
  printConsole,
  setTextPos,
  setTextSize,
  setTextString,
  storage,
  worldPointToScreenPoint,
} from 'skyrimPlatform';

import { ObjectReferenceEx } from '../extensions/objectReferenceEx';
import { RespawnNeededError } from '../lib/errors';
import { GamemodeUpdateService } from '../services/services/gamemodeUpdateService';
import { WorldCleanerService } from '../services/services/worldCleanerService';
import { SpApiInteractor } from '../services/spApiInteractor';
import { applyAnimation, setDefaultAnimsDisabled } from '../sync/animation';
import { Appearance, applyAppearance } from '../sync/appearance';
import { applyEquipment, isBadMenuShown } from '../sync/equipment';
import { applyMovement } from '../sync/movementApply';
import { getMovement } from '../sync/movementGet';
import { Movement } from '../sync/movement';
import { lastTryHost, tryHost } from './hostAttempts';
import { FormModel } from './model';
import { ModelApplyUtils } from './modelApplyUtils';
import { PlayerCharacterDataHolder } from './playerCharacterDataHolder';
import { SpawnProcess } from './spawnProcess';
import { localIdToRemoteId } from './worldViewMisc';

export interface ScreenResolution {
  width: number;
  height: number;
}

let _screenResolution: ScreenResolution | undefined;
export const getScreenResolution = (): ScreenResolution => {
  if (!_screenResolution) {
    _screenResolution = {
      width: Utility.getINIInt('iSize W:Display'),
      height: Utility.getINIInt('iSize H:Display'),
    };
  }
  return _screenResolution;
};

export class FormView {
  constructor(private remoteRefrId?: number) {}

  public static getMovementDebugStats() {
    return this.movementDebugStats;
  }

  update(model: FormModel): void {
    // Other players mutate into PC clones when moving to another location
    if (model.movement) {
      if (!this.lastWorldOrCell)
        this.lastWorldOrCell = model.movement.worldOrCell;
      if (this.lastWorldOrCell !== model.movement.worldOrCell) {
        printConsole(
          `[1] worldOrCell changed, destroying FormView ${this.lastWorldOrCell.toString(
            16,
          )} => ${model.movement.worldOrCell.toString(16)}`,
        );
        this.lastWorldOrCell = model.movement.worldOrCell;
        this.destroy();
        this.refrId = 0;
        this.appearanceBasedBaseId = 0;
        return;
      }
    }

    // Don't spawn dead actors if not already
    if (model.isDead) {
      if (this.refrId === 0) {
        return;
      }
    }

    // Players with different worldOrCell should be invisible
    if (model.movement) {
      const worldOrCell = ObjectReferenceEx.getWorldOrCell(
        Game.getPlayer() as Actor,
      );
      if (worldOrCell !== 0 && model.movement.worldOrCell !== worldOrCell) {
        this.destroy();
        this.refrId = 0;
        return;
      }
    }

    // Apply appearance before base form selection to prevent double-spawn
    if (
      model.appearance ||
      (!model.appearance && this.appearanceState.appearance)
    ) {
      if (
        !this.appearanceState.appearance ||
        model.numAppearanceChanges !== this.appearanceState.lastNumChanges
      ) {
        // Both non-null
        if (model.appearance && this.appearanceState.appearance) {
          const modelAppearanceCopy: Appearance = JSON.parse(
            JSON.stringify(model.appearance),
          );
          const stateAppearanceCopy: Appearance = JSON.parse(
            JSON.stringify(this.appearanceState.appearance),
          );
          modelAppearanceCopy.name = '';
          stateAppearanceCopy.name = '';
          const equalWithoutNames =
            JSON.stringify(modelAppearanceCopy) ===
            JSON.stringify(stateAppearanceCopy);

          if (equalWithoutNames) {
            // Change name inplace
            const refr = ObjectReference.from(Game.getFormEx(this.refrId));
            refr?.getBaseObject()?.setName(model.appearance.name);
            refr?.setDisplayName(model.appearance.name, true);
            //printConsole("Appearance updated, changing name inplace");
          } else {
            // Force re-apply appearance on the next getAppearanceBasedBase call
            this.appearanceBasedBaseId = 0;
            //printConsole("Appearance updated");
          }
        } else {
          // Force re-apply appearance on the next getAppearanceBasedBase call
          this.appearanceBasedBaseId = 0;
          //printConsole("Appearance updated");
        }

        this.appearanceState.appearance = model.appearance || null;
        this.appearanceState.lastNumChanges =
          model.numAppearanceChanges as number;
      }
    }

    const refId =
      model.refrId && model.refrId < 0xff000000 ? model.refrId : undefined;
    if (refId) {
      if (this.refrId !== refId) {
        this.destroy();
        this.refrId = model.refrId as number;
        this.ready = true;
        const refr = ObjectReference.from(Game.getFormEx(this.refrId));
        if (refr) {
          const base = refr.getBaseObject();
          if (base) {
            ObjectReferenceEx.dealWithRef(refr, base);
          }
        }
      }
    } else {
      let templateChain = model.templateChain;

      // There is no place for random/leveling in 1-sized chain
      // Just spawn an NPC, do not generate a temporary TESNPC form
      if (templateChain?.length === 1) {
        templateChain = undefined;
      }

      // TODO: getLeveledBase crashes too often ATM
      let base = null; //Game.getFormEx(this.getLeveledBase(templateChain));
      if (base === null) {
        base = Game.getFormEx(model.baseId || NaN);
      }
      if (base === null) {
        base = Game.getFormEx(this.getAppearanceBasedBase());
      }
      if (base === null) {
        return;
      }

      let refr = ObjectReference.from(Game.getFormEx(this.refrId));

      let respawnRequired = false;
      if (!refr) {
        respawnRequired = true;
      } else if (!refr.getBaseObject()) {
        respawnRequired = true;
      } else if (
        (refr.getBaseObject() as Form).getFormID() !== base.getFormID()
      ) {
        respawnRequired = true;
      }

      if (respawnRequired) {
        this.destroy();

        const player = Game.getPlayer() as Actor;

        const spawnMethodOriginal = {
          spawn(
            baseForm: Form,
            _spawnPosition: [number, number, number],
            _spawnRotation: [number, number, number],
          ): ObjectReference {
            return player.placeAtMe(baseForm, 1, true, true) as ObjectReference;
          },

          triggerSpawnProcess(
            spawningRefr: ObjectReference,
            spawnPosition: [number, number, number],
            appearance: Appearance | null,
            callback: () => void,
          ) {
            new SpawnProcess(
              appearance,
              spawnPosition,
              spawningRefr.getFormID(),
              callback,
            );
          },
        };

        const spawnMethodStub = {
          spawn(
            baseForm: Form,
            spawnPosition: [number, number, number],
            spawnRotation: [number, number, number],
          ): ObjectReference {
            const f = storage['formViewFunc1'] as Function;
            const ref: ObjectReference = f(
              baseForm,
              spawnPosition,
              spawnRotation,
            );
            return ref;
          },

          triggerSpawnProcess(
            spawningRefr: ObjectReference,
            spawnPosition: [number, number, number],
            appearance: Appearance | null,
            callback: () => void,
          ) {
            const f = storage['formViewFunc2'] as Function;
            f(spawningRefr, spawnPosition, appearance, callback);
          },
        };

        const spawnUsingStubMethod =
          base.getType() === FormType.NPC &&
          !this.appearanceState.appearance &&
          storage['formViewFunc1Set'] === true &&
          storage['formViewFunc2Set'] === true;
        const spawnMethod = spawnUsingStubMethod
          ? spawnMethodStub
          : spawnMethodOriginal;

        if (model.movement) {
          refr = spawnMethod.spawn(
            base,
            model.movement.pos,
            model.movement.rot,
          );
        } else {
          printConsole('model.movement was ' + model.movement);
        }

        this.state = {};
        delete this.wasHostedByOther;
        if (base.getType() !== FormType.NPC) {
          refr?.setAngle(
            model.movement?.rot[0] || 0,
            model.movement?.rot[1] || 0,
            model.movement?.rot[2] || 0,
          );
        } else {
          const race = Actor.from(refr)?.getRace()?.getFormID();
          const draugrRace = 0xd53;
          const falmerRace = 0x131f4;
          const chaurusRace = 0x131eb;
          const frostbiteSpiderRaceGiant = 0x4e507;
          const frostbiteSpiderRaceLarge = 0x53477;
          const dwarvenCenturionRace = 0x131f1;
          const dwarvenSphereRace = 0x131f2;
          const dwarvenSpiderRace = 0x131f3;
          const sprigganRace = 0x2013b77;
          const sprigganRace2 = 0xf3903;
          const sprigganRace3 = 0x13204;
          const sprigganRace4 = 0x401b644;
          const sprigganRace5 = 0x9aa44;
          const wolfRace = 0x1320a;

          // potential masterambushscript
          if (
            race === draugrRace ||
            race === falmerRace ||
            race === chaurusRace ||
            race === frostbiteSpiderRaceGiant ||
            race === frostbiteSpiderRaceLarge ||
            race === dwarvenCenturionRace ||
            race === dwarvenSphereRace ||
            race === dwarvenSpiderRace ||
            race === sprigganRace ||
            race === sprigganRace2 ||
            race === sprigganRace3 ||
            race === sprigganRace4 ||
            race === sprigganRace5 ||
            race === wolfRace
          ) {
            Actor.from(refr)?.setActorValue('Aggression', 2);
          }
        }

        if (refr !== null) {
          SpApiInteractor.getControllerInstance()
            .lookupListener(WorldCleanerService)
            .modWcProtection(refr.getFormID(), 1);
        }

        // TODO: reset all states?
        this.eqState = this.getDefaultEquipState();
        this.animState = this.getDefaultAnimState();

        this.ready = false;

        let spawnPos;
        if (model.movement) {
          spawnPos = model.movement.pos;
          // printConsole("Spawn NPC at movement.pos");
        } else {
          spawnPos = ObjectReferenceEx.getPos(Game.getPlayer() as Actor);
          printConsole('Spawn NPC at player pos');
        }

        if (refr) {
          spawnMethod.triggerSpawnProcess(
            refr,
            spawnPos,
            model.appearance || null,
            () => {
              this.ready = true;
              this.spawnMoment = Date.now();
            },
          );
        } else {
          printConsole('Unable to triggerSpawnProcess for null refr');
        }

        if (model.appearance && model.appearance.name) {
          refr?.setDisplayName('' + model.appearance.name, true);
        }
        Actor.from(refr)?.setActorValue('attackDamageMult', 0);
      }
      this.refrId = (refr as ObjectReference).getFormID();
    }

    if (!this.ready) {
      return;
    }

    const refr = ObjectReference.from(Game.getFormEx(this.refrId));
    if (refr) {
      const actor = Actor.from(refr);
      if (actor && !this.localImmortal) {
        actor.startDeferredKill();
        actor.setActorValue('health', 1000000);
        actor.setActorValue('magicka', 1000000);
        this.localImmortal = true;
      }
      this.applyAll(refr, model);

      const gamemodeUpdateService =
        SpApiInteractor.getControllerInstance().lookupListener(
          GamemodeUpdateService,
        );
      gamemodeUpdateService.updateNeighbor(refr, model, this.state);
    }
  }

  destroy(): void {
    this.isOnScreen = false;
    this.spawnMoment = 0;
    const refrId = this.refrId;
    once('update', () => {
      if (refrId >= 0xff000000) {
        const refr = ObjectReference.from(Game.getFormEx(refrId));
        if (refr) {
          refr.delete();
        }
        SpApiInteractor.getControllerInstance()
          .lookupListener(WorldCleanerService)
          .modWcProtection(refrId, -1);
        const ac = Actor.from(refr);
        if (ac) {
          TESModPlatform.setWeaponDrawnMode(ac, -1);
        }
      }
    });

    this.localImmortal = false;
    this.removeNickname();
  }

  private lastHarvestedApply = 0;
  private lastOpenApply = 0;
  private isSetNodeTextureSetApplied = false;
  private isSetNodeScaleApplied = false;

  private applyAll(refr: ObjectReference, model: FormModel) {
    let forcedWeapDrawn: boolean | null = null;

    if (PlayerCharacterDataHolder.getCrosshairRefId() === this.refrId) {
      this.lastHarvestedApply = 0;
      this.lastOpenApply = 0;
    }
    const now = Date.now();
    if (now - this.lastHarvestedApply > 666) {
      this.lastHarvestedApply = now;
      ModelApplyUtils.applyModelIsHarvested(refr, !!model.isHarvested);
    }
    if (now - this.lastOpenApply > 133) {
      this.lastOpenApply = now;
      ModelApplyUtils.applyModelIsOpen(refr, !!model.isOpen);
    }
    if (!this.isSetNodeScaleApplied) {
      this.isSetNodeScaleApplied = true;
      ModelApplyUtils.applyModelNodeScale(refr, model.setNodeScale);
    }
    if (!this.isSetNodeTextureSetApplied) {
      this.isSetNodeTextureSetApplied = true;
      ModelApplyUtils.applyModelNodeTextureSet(refr, model.setNodeTextureSet);
    }

    if (
      model.inventory &&
      PlayerCharacterDataHolder.getCrosshairRefId() == this.refrId &&
      !isBadMenuShown()
    ) {
      // Do not let actors breaking their equipment via inventory apply
      // However, actually, actors do not have inventory in their models
      // Except your clone.
      if (!Actor.from(refr)) {
        ModelApplyUtils.applyModelInventory(refr, model.inventory);
        model.inventory = undefined;
      }
    }

    if (model.animation) {
      if (model.animation.animEventName === 'SkympFakeUnequip') {
        forcedWeapDrawn = false;
      } else if (model.animation.animEventName === 'SkympFakeEquip') {
        forcedWeapDrawn = true;
      }
    }

    // TODO: make host service
    const hosted = storage['hosted'];
    let alreadyHosted = false;
    if (Array.isArray(hosted)) {
      const remoteId = localIdToRemoteId(this.refrId);

      if (
        hosted.includes(remoteId) ||
        hosted.includes(remoteId + 0x100000000)
      ) {
        alreadyHosted = true;
      }
    }
    setDefaultAnimsDisabled(this.refrId, alreadyHosted ? false : true);

    if (alreadyHosted) {
      Actor.from(refr)?.clearKeepOffsetFromActor();
    }

    if (model.movement) {
      let ac = Actor.from(refr);
      const now = Date.now();
      const hasNewMovement =
        +(model.numMovementChanges as number) !== this.movState.lastNumChanges;

      if (hasNewMovement) {
        this.registerMovementPacket(model.movement, now);
        this.movState.lastNumChanges = +(model.numMovementChanges as number);
      }

      if (
        this.movState.lastApply &&
        now - this.movState.lastApply > 1500
      ) {
        if (now - this.movState.lastRehost > 1000) {
          this.movState.lastRehost = now;
          const remoteId = this.remoteRefrId;
          if (ac && ac.is3DLoaded()) {
            this.tryHostIfNeed(ac, remoteId as number);
            printConsole(
              'tryHostIfNeed - reason: not seeing movement for long time',
            );
          }
        }
      }

      if (
        hasNewMovement ||
        now - this.movState.lastApply > 33
      ) {
        if (model.isHostedByOther || !this.movState.everApplied) {
          const backup = model.movement.isWeapDrawn;
          let movementToApply: Movement;
          if (forcedWeapDrawn === true || forcedWeapDrawn === false) {
            model.movement.isWeapDrawn = forcedWeapDrawn;
          }
          try {
            movementToApply = this.getBufferedMovement(model.movement, now);
            applyMovement(refr, movementToApply, !!model.isMyClone);
          } catch (e) {
            if (e instanceof RespawnNeededError) {
              this.lastWorldOrCell = model.movement.worldOrCell;
              this.destroy();
              this.refrId = 0;
              this.appearanceBasedBaseId = 0;
              return;
            } else {
              throw e;
            }
          }
          model.movement.isWeapDrawn = backup;

          this.movState.lastAppliedMovement = movementToApply;
          this.movState.lastApply = now;
          this.movState.everApplied = true;
        } else {
          const remoteId = this.remoteRefrId;
          if (ac && remoteId && ac.is3DLoaded()) {
            ac.clearKeepOffsetFromActor();

            // TODO: make host service
            const hosted = storage['hosted'];
            let alreadyHosted = false;
            if (Array.isArray(hosted)) {
              const remoteId = localIdToRemoteId(ac.getFormID());
              if (
                hosted.includes(remoteId) ||
                hosted.includes(remoteId + 0x100000000)
              ) {
                alreadyHosted = true;
              }
            }

            if (!alreadyHosted) {
              if (this.tryHostIfNeed(ac, remoteId)) {
                // previously, we did this cleanup on each update
                // but I guess it's too expensive and can possibly hurt FPS
                TESModPlatform.setWeaponDrawnMode(ac, -1);
              }
            }
          }
        }
      }
    }

    if (refr.is3DLoaded()) {
      if (model.animation) {
        applyAnimation(refr, model.animation, this.animState);
      }
      // Use them only once, for spawning actors with correct animations
      this.animState.useAnimOverrides = false;
    }

    if (model.appearance) {
      const actor = Actor.from(refr);
      if (actor && !PlayerCharacterDataHolder.isInJumpState()) {
        if (PlayerCharacterDataHolder.getWorldOrCell()) {
          if (
            this.lastPcWorldOrCell &&
            PlayerCharacterDataHolder.getWorldOrCell() !==
              this.lastPcWorldOrCell
          ) {
            // Redraw tints if PC world/cell changed
            this.isOnScreen = false;
          }
          this.lastPcWorldOrCell = PlayerCharacterDataHolder.getWorldOrCell();
        }

        const headPos = [
          NetImmerse.getNodeWorldPositionX(actor, 'NPC Head [Head]', false),
          NetImmerse.getNodeWorldPositionY(actor, 'NPC Head [Head]', false),
          NetImmerse.getNodeWorldPositionZ(actor, 'NPC Head [Head]', false),
        ];
        const [screenPoint] = worldPointToScreenPoint(headPos);
        const isOnScreen =
          screenPoint[0] > 0 &&
          screenPoint[1] > 0 &&
          screenPoint[2] > 0 &&
          screenPoint[0] < 1 &&
          screenPoint[1] < 1 &&
          screenPoint[2] < 1;
        if (isOnScreen != this.isOnScreen) {
          this.isOnScreen = isOnScreen;
          if (isOnScreen) {
            actor.queueNiNodeUpdate();
            (Game.getPlayer() as Actor).queueNiNodeUpdate();
          }
        }
      }
    }

    if (model.equipment) {
      if (this.eqState.lastNumChanges !== model.equipment.numChanges) {
        const ac = Actor.from(refr);
        // If we do not block inventory here, we will be able to reproduce the bug:
        // 1. Place ~90 bots and force them to reequip iron swords to the left hand (rate should be ~50ms)
        // 2. Open your inventory and reequip different items fast
        // 3. After 1-2 minutes close your inventory and see that HUD disappeared
        if (
          ac &&
          !isBadMenuShown() &&
          Date.now() - this.eqState.lastEqMoment > 500 &&
          Date.now() - this.spawnMoment > -1 &&
          this.spawnMoment > 0
        ) {
          //if (this.spawnMoment > 0 && Date.now() - this.spawnMoment > 5000) {
          if (applyEquipment(ac, model.equipment)) {
            this.eqState.lastNumChanges = model.equipment.numChanges;
          }
          this.eqState.lastEqMoment = Date.now();
          //}
          //const res: boolean = applyEquipment(ac, model.equipment);
          //if (res) this.eqState.lastNumChanges = model.equipment.numChanges;
        }
      }
    }

    if (
      FormView.isDisplayingNicknames &&
      this.refrId &&
      model.appearance?.name
    ) {
      const headPart = 'NPC Head [Head]';
      const maxNicknameDrawDistance = 1000;
      const playerActor = Game.getPlayer()!;
      const isVisibleByPlayer =
        !model.movement?.isSneaking &&
        playerActor.getDistance(refr) <= maxNicknameDrawDistance &&
        playerActor.hasLOS(refr) &&
        !this.isSweetHidePerson(refr);
      if (isVisibleByPlayer) {
        const headScreenPos = worldPointToScreenPoint([
          NetImmerse.getNodeWorldPositionX(refr, headPart, false),
          NetImmerse.getNodeWorldPositionY(refr, headPart, false),
          NetImmerse.getNodeWorldPositionZ(refr, headPart, false) + 32,
        ])[0];
        const resolution = getScreenResolution();
        const textXPos = Math.round(headScreenPos[0] * resolution.width);
        const textYPos = Math.round((1 - headScreenPos[1]) * resolution.height);

        if (!this.textNameId && headScreenPos[2] > 0) {
          this.textNameId = createText(
            textXPos,
            textYPos,
            refr.getDisplayName(),
            [1, 1, 1, 0.8],
          );
          setTextSize(this.textNameId, 0.5);
          SpApiInteractor.getControllerInstance().emitter.emit(
            'nicknameCreate',
            {
              remoteRefrId: this.getRemoteRefrId(),
              textId: this.textNameId,
            },
          );
        } else {
          const deleteNickname = headScreenPos[2] < 0;
          if (deleteNickname) {
            this.removeNickname();
          }
          if (this.textNameId) {
            setTextPos(this.textNameId, textXPos, textYPos);
          }
        }
      } else {
        this.removeNickname();
      }
    } else {
      this.removeNickname();
    }
  }

  private isSweetHidePerson(refr: ObjectReference): boolean {
    const actor = Actor.from(refr);
    if (!actor) {
      return false;
    }
    const keyword = Keyword.getKeyword('SweetHidePerson');
    return actor.wornHasKeyword(keyword);
  }

  private removeNickname() {
    if (this.textNameId) {
      SpApiInteractor.getControllerInstance().emitter.emit('nicknameDestroy', {
        remoteRefrId: this.getRemoteRefrId(),
        textId: this.textNameId,
      });
      destroyText(this.textNameId);
      this.textNameId = undefined;
    }
  }

  private getAppearanceBasedBase(): number {
    const base = ActorBase.from(Game.getFormEx(this.appearanceBasedBaseId));
    if (!base && this.appearanceState.appearance) {
      this.appearanceBasedBaseId = applyAppearance(
        this.appearanceState.appearance,
      ).getFormID();
    }
    return this.appearanceBasedBaseId;
  }

  private getLeveledBase(templateChain: number[] | undefined): number {
    if (templateChain === undefined) {
      return 0;
    }

    const str = templateChain.join(',');

    if (this.leveledBaseId === 0) {
      // @ts-ignore
      const leveledBase = TESModPlatform.evaluateLeveledNpc(str);
      if (!leveledBase) {
        printConsole('Failed to evaluate leveled npc', str);
      }
      this.leveledBaseId = leveledBase?.getFormID() || 0;
    }

    return this.leveledBaseId;
  }

  private getDefaultEquipState() {
    return { lastNumChanges: 0, lastEqMoment: 0 };
  }

  private getDefaultAppearanceState() {
    return { lastNumChanges: 0, appearance: null as null | Appearance };
  }

  private getDefaultAnimState() {
    return { lastNumChanges: 0, useAnimOverrides: true };
  }

  private registerMovementPacket(movement: Movement, receivedAt: number): void {
    const latestMovement = this.movState.latestMovement;
    const latestReceivedAt = this.movState.latestMovementReceivedAt;

    if (this.movState.hasLatestMovement && latestMovement && latestReceivedAt > 0) {
      const interval = receivedAt - latestReceivedAt;
      if (interval > 0) {
        const clampedInterval = Math.max(50, Math.min(200, interval));
        this.movState.averagePacketIntervalMs =
          this.movState.averagePacketIntervalMs > 0
            ? this.lerp(
                this.movState.averagePacketIntervalMs,
                clampedInterval,
                0.35,
              )
            : clampedInterval;
      }
      this.copyMovement(this.movState.previousMovement, latestMovement);
      this.movState.hasPreviousMovement = true;
      this.movState.previousMovementReceivedAt = latestReceivedAt;
    }

    this.copyMovement(this.movState.latestMovement, movement);
    this.movState.hasLatestMovement = true;
    this.movState.latestMovementReceivedAt = receivedAt;
  }

  private getBufferedMovement(movement: Movement, now: number): Movement {
    const previousMovement = this.movState.previousMovement;
    const latestMovement = this.movState.latestMovement;
    if (!this.movState.hasPreviousMovement) {
      FormView.movementDebugStats.averagePacketIntervalMs =
        this.movState.averagePacketIntervalMs;
      FormView.movementDebugStats.extrapolationMs = 0;
      FormView.movementDebugStats.lastSnapDistance = 0;
      return this.getFallbackSmoothedMovement(
        this.movState.hasLatestMovement ? latestMovement : movement,
      );
    }

    if (previousMovement.worldOrCell !== latestMovement.worldOrCell) {
      FormView.movementDebugStats.averagePacketIntervalMs =
        this.movState.averagePacketIntervalMs;
      FormView.movementDebugStats.extrapolationMs = 0;
      FormView.movementDebugStats.lastSnapDistance = 0;
      return latestMovement;
    }

    const distance = ObjectReferenceEx.getDistance(
      previousMovement.pos,
      latestMovement.pos,
    );
    if (distance > 256) {
      FormView.movementDebugStats.averagePacketIntervalMs =
        this.movState.averagePacketIntervalMs;
      FormView.movementDebugStats.extrapolationMs = 0;
      FormView.movementDebugStats.lastSnapDistance = distance;
      FormView.movementDebugStats.hardCorrectionCount++;
      return latestMovement;
    }

    const previousReceivedAt = this.movState.previousMovementReceivedAt;
    const latestReceivedAt = this.movState.latestMovementReceivedAt;
    const packetInterval = this.movState.averagePacketIntervalMs || 100;
    const renderBackTime = Math.min(90, Math.max(55, packetInterval * 0.75));
    const renderTime = now - renderBackTime;
    const rawBlend = (renderTime - previousReceivedAt) / packetInterval;
    const blend = Math.max(
      0,
      Math.min(1, rawBlend),
    );

    const interpolatedMovement = this.movState.interpolatedMovement;
    this.copyMovement(interpolatedMovement, latestMovement);
    interpolatedMovement.pos[0] = this.lerp(
      previousMovement.pos[0],
      latestMovement.pos[0],
      blend,
    );
    interpolatedMovement.pos[1] = this.lerp(
      previousMovement.pos[1],
      latestMovement.pos[1],
      blend,
    );
    interpolatedMovement.pos[2] = this.lerp(
      previousMovement.pos[2],
      latestMovement.pos[2],
      blend,
    );
    interpolatedMovement.rot[0] = this.lerp(
      previousMovement.rot[0],
      latestMovement.rot[0],
      blend,
    );
    interpolatedMovement.rot[1] = this.lerp(
      previousMovement.rot[1],
      latestMovement.rot[1],
      blend,
    );
    interpolatedMovement.rot[2] = this.lerpAngle(
      previousMovement.rot[2],
      latestMovement.rot[2],
      blend,
    );
    interpolatedMovement.direction = this.lerpAngle(
      previousMovement.direction,
      latestMovement.direction,
      blend,
    );

    const extrapolationMs = Math.min(
      60,
      Math.max(0, renderTime - latestReceivedAt),
    );
    FormView.movementDebugStats.averagePacketIntervalMs = packetInterval;
    FormView.movementDebugStats.extrapolationMs = extrapolationMs;
    FormView.movementDebugStats.lastSnapDistance = 0;
    if (extrapolationMs <= 0) {
      return interpolatedMovement;
    }

    return {
      ...interpolatedMovement,
      pos: this.extrapolateMovementPos(interpolatedMovement, extrapolationMs),
    };
  }

  private getFallbackSmoothedMovement(movement: Movement): Movement {
    const lastMovement = this.movState.lastAppliedMovement;
    if (!lastMovement) {
      return movement;
    }

    if (lastMovement.worldOrCell !== movement.worldOrCell) {
      return movement;
    }

    const distance = ObjectReferenceEx.getDistance(lastMovement.pos, movement.pos);
    if (distance > 256) {
      return movement;
    }

    const blend = movement.runMode === 'Standing' && !movement.isInJumpState
      ? 0.35
      : 0.6;

    const fallbackMovement = this.movState.interpolatedMovement;
    this.copyMovement(fallbackMovement, movement);
    fallbackMovement.pos[0] = this.lerp(lastMovement.pos[0], movement.pos[0], blend);
    fallbackMovement.pos[1] = this.lerp(lastMovement.pos[1], movement.pos[1], blend);
    fallbackMovement.pos[2] = this.lerp(lastMovement.pos[2], movement.pos[2], blend);
    fallbackMovement.rot[0] = this.lerp(lastMovement.rot[0], movement.rot[0], blend);
    fallbackMovement.rot[1] = this.lerp(lastMovement.rot[1], movement.rot[1], blend);
    fallbackMovement.rot[2] = this.lerpAngle(lastMovement.rot[2], movement.rot[2], blend);
    fallbackMovement.direction = this.lerpAngle(lastMovement.direction, movement.direction, blend);
    return fallbackMovement;
  }

  private lerp(from: number, to: number, blend: number): number {
    return from + (to - from) * blend;
  }

  private lerpAngle(from: number, to: number, blend: number): number {
    let delta = ((to - from + 540) % 360) - 180;
    if (Number.isNaN(delta)) {
      delta = 0;
    }
    return from + delta * blend;
  }

  private extrapolateMovementPos(
    movement: Movement,
    extrapolationMs: number,
  ): [number, number, number] {
    if (movement.runMode === 'Standing' || movement.isInJumpState) {
      return movement.pos;
    }

    const distanceAdd = movement.speed * (extrapolationMs / 1000);
    const direction = movement.rot[2] + movement.direction;
    const extrapolatedPos = this.movState.extrapolatedPos;
    extrapolatedPos[0] =
      movement.pos[0] + Math.sin((direction / 180) * Math.PI) * distanceAdd;
    extrapolatedPos[1] =
      movement.pos[1] + Math.cos((direction / 180) * Math.PI) * distanceAdd;
    extrapolatedPos[2] = movement.pos[2];
    return extrapolatedPos;
  }

  private copyMovement(target: Movement, source: Movement): void {
    target.worldOrCell = source.worldOrCell;
    target.pos[0] = source.pos[0];
    target.pos[1] = source.pos[1];
    target.pos[2] = source.pos[2];
    target.rot[0] = source.rot[0];
    target.rot[1] = source.rot[1];
    target.rot[2] = source.rot[2];
    target.runMode = source.runMode;
    target.direction = source.direction;
    target.isInJumpState = source.isInJumpState;
    target.isMounted = source.isMounted;
    target.mountRemoteId = source.mountRemoteId;
    target.isSneaking = source.isSneaking;
    target.isBlocking = source.isBlocking;
    target.isWeapDrawn = source.isWeapDrawn;
    target.isDead = source.isDead;
    target.healthPercentage = source.healthPercentage;
    target.speed = source.speed;
    if (source.lookAt) {
      target.lookAt = target.lookAt ?? [0, 0, 0];
      target.lookAt[0] = source.lookAt[0];
      target.lookAt[1] = source.lookAt[1];
      target.lookAt[2] = source.lookAt[2];
    } else {
      target.lookAt = undefined;
    }
  }

  private createMovementBuffer(): Movement {
    return {
      worldOrCell: 0,
      pos: [0, 0, 0],
      rot: [0, 0, 0],
      runMode: 'Standing',
      direction: 0,
      isInJumpState: false,
      isMounted: false,
      isSneaking: false,
      isBlocking: false,
      isWeapDrawn: false,
      isDead: false,
      healthPercentage: 1,
      speed: 0,
    };
  }

  private tryHostIfNeed(ac: Actor, remoteId: number) {
    const last = lastTryHost[remoteId];
    if (!last || Date.now() - last >= 1000) {
      lastTryHost[remoteId] = Date.now();

      if (
        getMovement(ac).worldOrCell ===
        getMovement(Game.getPlayer() as Actor).worldOrCell
      ) {
        tryHost(remoteId);
        return true;
      }
    }
    return false;
  }

  getLocalRefrId(): number {
    return this.refrId;
  }

  getRemoteRefrId(): number {
    return this.remoteRefrId as number;
  }

  private refrId = 0;
  private ready = false;
  private animState = this.getDefaultAnimState();
  private movState = {
    lastNumChanges: 0,
    lastApply: 0,
    lastRehost: 0,
    everApplied: false,
    lastAppliedMovement: undefined as Movement | undefined,
    previousMovement: this.createMovementBuffer(),
    latestMovement: this.createMovementBuffer(),
    interpolatedMovement: this.createMovementBuffer(),
    hasPreviousMovement: false,
    hasLatestMovement: false,
    previousMovementReceivedAt: 0,
    latestMovementReceivedAt: 0,
    averagePacketIntervalMs: 0,
    extrapolatedPos: [0, 0, 0] as [number, number, number],
  };
  private appearanceState = this.getDefaultAppearanceState();
  private eqState = this.getDefaultEquipState();
  private appearanceBasedBaseId = 0;
  private leveledBaseId = 0;
  private isOnScreen = false;
  private lastPcWorldOrCell = 0;
  private lastWorldOrCell = 0;
  private spawnMoment = 0;
  private wasHostedByOther: boolean | undefined = undefined;
  private state = {};
  private localImmortal = false;
  private textNameId: number | undefined = undefined;

  public static isDisplayingNicknames: boolean = true;
  public static isNicknameDisplayServerControlled: boolean = false;
  private static movementDebugStats = {
    averagePacketIntervalMs: 0,
    extrapolationMs: 0,
    lastSnapDistance: 0,
    hardCorrectionCount: 0,
  };
}
