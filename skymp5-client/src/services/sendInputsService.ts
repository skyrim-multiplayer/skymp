import { ClientListener, CombinedController, Sp } from "./clientListener";
import { SinglePlayerService } from "./singlePlayerService";
import { FormModel } from "../modelSource/model";
import { MsgType } from "../messages";
import { ModelSource } from "../modelSource/modelSource";
import { getMovement } from "../sync/movementGet";

// TODO: refactor this out
import * as worldViewMisc from "../view/worldViewMisc";

import { Animation, AnimationSource } from "../sync/animation";
import { Actor, EquipEvent } from "skyrimPlatform";
import { getAppearance } from "../sync/appearance";
import { ActorValues, getActorValues } from "../sync/actorvalues";
import { getEquipment } from "../sync/equipment";
import { nextHostAttempt } from "../view/hostAttempts";
import { SkympClient } from "../skympClient";

const playerFormId = 0x14;

export class SendInputsService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        this.controller.on("update", () => this.onUpdate());
        this.controller.on("equip", (e) => this.onEquip(e));
        this.controller.on("unequip", (e) => this.onUnequip(e));
        this.controller.on("loadGame", () => this.onLoadGame());
        this.singlePlayerService = this.controller.lookupListener("SinglePlayerService") as SinglePlayerService;
    }

    private onUpdate() {
        if (!this.singlePlayerService.isSinglePlayer) {
            this.sendInputs();
        }
    }

    private onEquip(event: EquipEvent) {
        const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;

        if (!event.actor || !event.baseObj) {
            return;
        }

        if (event.actor.getFormID() === playerFormId) {
            this.equipmentChanged = true;

            skympClient.getSendTarget().send(
                { t: MsgType.OnEquip, baseId: event.baseObj.getFormID() },
                false,
            );
        }
    }

    private onUnequip(event: EquipEvent) {
        if (!event.actor || !event.baseObj) {
            return;
        }

        if (event.actor.getFormID() === playerFormId) {
            this.equipmentChanged = true;
        }
    }

    private onLoadGame() {
        // Currently only armor is equipped after relogging (see remoteServer.ts)
        // This hack forces sending /equipment without weapons/ back to the server
        this.sp.Utility.wait(3).then(() => (this.equipmentChanged = true));
    }

    private sendInputs() {
        const hosted =
            typeof this.sp.storage['hosted'] === typeof [] ? this.sp.storage['hosted'] : [];
        const targets = [undefined].concat(hosted as any);
        
        targets.forEach((target) => {
            this.sendMovement(target);
            this.sendAnimation(target);
            this.sendAppearance(target);
            this.sendEquipment(target);
            this.sendActorValuePercentage(
                target,
                target ? this.getForm(target) : this.getForm(),
            );
        });
        this.sendHostAttempts();
    }

    private sendMovement(_refrId?: number) {
        const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;

        const owner = this.getInputOwner(_refrId);
        if (!owner) return;

        const refrIdStr = `${_refrId}`;
        const sendMovementRateMs = 130;
        const now = Date.now();
        const last = this.lastSendMovementMoment.get(refrIdStr);
        if (!last || now - last > sendMovementRateMs) {
            skympClient.getSendTarget().send(
                {
                    t: MsgType.UpdateMovement,
                    data: getMovement(owner, this.getForm(_refrId)),
                    _refrId,
                },
                false,
            );
            this.lastSendMovementMoment.set(refrIdStr, now);
        }
    }

    private sendActorValuePercentage(_refrId?: number, form?: FormModel) {
        const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;

        const canSend = form && (form.isDead ?? false) === false;
        if (!canSend) return;

        const owner = this.getInputOwner(_refrId);
        if (!owner) return;

        const av = getActorValues(this.sp.Game.getPlayer() as Actor);
        const currentTime = Date.now();
        if (
            this.actorValuesNeedUpdate === false &&
            this.prevValues.health === av.health &&
            this.prevValues.stamina === av.stamina &&
            this.prevValues.magicka === av.magicka
        ) {
            return;
        } else {
            if (
                currentTime - this.prevActorValuesUpdateTime < 1000 &&
                this.actorValuesNeedUpdate === false
            ) {
                return;
            }
            skympClient.getSendTarget().send(
                { t: MsgType.ChangeValues, data: av, _refrId },
                true,
            );
            this.actorValuesNeedUpdate = false;
            this.prevValues = av;
            this.prevActorValuesUpdateTime = currentTime;
        }
    }

    private sendAnimation(_refrId?: number) {
        const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;

        const owner = this.getInputOwner(_refrId);
        if (!owner) return;

        // Extermly important that it's a local id since AnimationSource depends on it
        const refrIdStr = owner.getFormID().toString(16);

        let animSource = this.playerAnimSource.get(refrIdStr);
        if (!animSource) {
            animSource = new AnimationSource(owner);
            this.playerAnimSource.set(refrIdStr, animSource);
        }
        const anim = animSource.getAnimation();

        const lastAnimationSent = this.lastAnimationSent.get(refrIdStr);
        if (
            !lastAnimationSent ||
            anim.numChanges !== lastAnimationSent.numChanges
        ) {
            if (anim.animEventName !== '') {
                this.lastAnimationSent.set(refrIdStr, anim);
                this.updateActorValuesAfterAnimation(anim.animEventName);
                skympClient.getSendTarget().send(
                    { t: MsgType.UpdateAnimation, data: anim, _refrId },
                    false,
                );
            }
        }
    }

    private sendAppearance(_refrId?: number) {
        const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;

        if (_refrId) return;
        const shown = this.sp.Ui.isMenuOpen('RaceSex Menu');
        if (shown != this.isRaceSexMenuShown) {
            this.isRaceSexMenuShown = shown;
            if (!shown) {
                this.sp.printConsole('Exited from race menu');

                const appearance = getAppearance(this.sp.Game.getPlayer() as Actor);
                skympClient.getSendTarget().send(
                    { t: MsgType.UpdateAppearance, data: appearance, _refrId },
                    true,
                );
            }
        }
    }

    private sendEquipment(_refrId?: number) {
        const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;

        if (_refrId) return;
        if (this.equipmentChanged) {
            this.equipmentChanged = false;

            ++this.numEquipmentChanges;

            const eq = getEquipment(
                this.sp.Game.getPlayer() as Actor,
                this.numEquipmentChanges,
            );
            skympClient.getSendTarget().send(
                { t: MsgType.UpdateEquipment, data: eq, _refrId },
                true,
            );
        }
    }

    private sendHostAttempts() {
        const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;

        const remoteId = nextHostAttempt();
        if (!remoteId) return;

        skympClient.getSendTarget().send({ t: MsgType.Host, remoteId }, false);
    }

    private getInputOwner(_refrId?: number) {
        return _refrId
            ? this.sp.Actor.from(this.sp.Game.getFormEx(worldViewMisc.remoteIdToLocalId(_refrId)))
            : this.sp.Game.getPlayer();
    }

    private getForm(refrId?: number): FormModel | undefined {
        const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;

        const world = (skympClient.getModelSource() as ModelSource).getWorldModel();
        const form = refrId
            ? world?.forms.find((f) => f?.refrId === refrId)
            : world.forms[world.playerCharacterFormIdx];
        return form;
    }

    private updateActorValuesAfterAnimation(animName: string) {
        if (
            animName === 'JumpLand' ||
            animName === 'JumpLandDirectional' ||
            animName === 'DeathAnim'
        ) {
            this.actorValuesNeedUpdate = true;
        }
    }

    private singlePlayerService: SinglePlayerService;

    private lastSendMovementMoment = new Map<string, number>();
    private playerAnimSource = new Map<string, AnimationSource>(); // TODO: make service
    private lastAnimationSent = new Map<string, Animation>();
    private actorValuesNeedUpdate = false;
    private isRaceSexMenuShown = false;
    private equipmentChanged = false;
    private numEquipmentChanges = 0;
    private prevValues: ActorValues = { health: 0, stamina: 0, magicka: 0 };
    private prevActorValuesUpdateTime = 0;
}
