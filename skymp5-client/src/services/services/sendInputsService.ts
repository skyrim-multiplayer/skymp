import { ClientListener, CombinedController, Sp } from "./clientListener";
import { SinglePlayerService } from "./singlePlayerService";
import { FormModel, WorldModel } from "../../modelSource/model";
import { MsgType } from "../../messages";
import { getMovement } from "../../sync/movementGet";

// TODO: refactor this out
import * as worldViewMisc from "../../view/worldViewMisc";

import { Animation, AnimationSource } from "../../sync/animation";
import { Actor, EquipEvent } from "skyrimPlatform";
import { getAppearance } from "../../sync/appearance";
import { ActorValues, getActorValues } from "../../sync/actorvalues";
import { getEquipment } from "../../sync/equipment";
import { nextHostAttempt } from "../../view/hostAttempts";
import { SkympClient } from "./skympClient";
import { MessageWithRefrId } from "../events/sendMessageWithRefrIdEvent";
import { UpdateMovementMessage } from "../messages/updateMovementMessage";
import { ChangeValuesMessage } from "../messages/changeValues";
import { UpdateAnimationMessage } from "../messages/updateAnimationMessage";
import { UpdateEquipmentMessage } from "../messages/updateEquipmentMessage";
import { UpdateAppearanceMessage } from "../messages/updateAppearanceMessage";

const playerFormId = 0x14;

// TODO: split this service into EquipmentService, MovementService, AnimationService, ActorValueService, HostAttemptsService
export class SendInputsService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.on("update", () => this.onUpdate());
        this.controller.on("equip", (e) => this.onEquip(e));
        this.controller.on("unequip", (e) => this.onUnequip(e));
        this.controller.on("loadGame", () => this.onLoadGame());
    }

    private onUpdate() {
        if (!this.singlePlayerService.isSinglePlayer) {
            this.sendInputs();
        }
    }

    private onEquip(event: EquipEvent) {
        if (!event.actor || !event.baseObj) {
            return;
        }

        if (event.actor.getFormID() === playerFormId) {
            this.equipmentChanged = true;

            this.controller.emitter.emit("sendMessage", {
                message: { t: MsgType.OnEquip, baseId: event.baseObj.getFormID() },
                reliability: "unreliable"
            });
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

        const skympClient = this.controller.lookupListener(SkympClient);
        const modelSource = skympClient.modelSource;
        if (!modelSource) {
            return;
        }

        const world = modelSource.getWorldModel();

        targets.forEach((target) => {
            const targetFormModel = target ? this.getForm(target, world) : this.getForm(undefined, world);
            this.sendMovement(target, targetFormModel);
            this.sendAnimation(target);
            this.sendAppearance(target);
            this.sendEquipment(target);
            this.sendActorValuePercentage(target, targetFormModel);
        });
        this.sendHostAttempts();
    }

    private sendMovement(_refrId?: number, form?: FormModel) {
        const owner = this.getInputOwner(_refrId);
        if (!owner) return;

        const refrIdStr = `${_refrId}`;
        const sendMovementRateMs = 130;
        const now = Date.now();
        const last = this.lastSendMovementMoment.get(refrIdStr);
        if (!last || now - last > sendMovementRateMs) {
            const message: MessageWithRefrId<UpdateMovementMessage> = {
                t: MsgType.UpdateMovement,
                data: getMovement(owner, form),
                _refrId
            };
            this.controller.emitter.emit("sendMessageWithRefrId", {
                message,
                reliability: "unreliable"
            });
            this.lastSendMovementMoment.set(refrIdStr, now);
        }
    }

    private sendActorValuePercentage(_refrId?: number, form?: FormModel) {
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
                currentTime - this.prevActorValuesUpdateTime < 2000 &&
                this.actorValuesNeedUpdate === false
            ) {
                return;
            }
            const message: MessageWithRefrId<ChangeValuesMessage> = {
                t: MsgType.ChangeValues,
                data: av,
                _refrId
            };
            this.controller.emitter.emit("sendMessageWithRefrId", {
                message,
                reliability: "unreliable"
            });
            this.actorValuesNeedUpdate = false;
            this.prevValues = av;
            this.prevActorValuesUpdateTime = currentTime;
        }
    }

    private sendAnimation(_refrId?: number) {
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
                const message: MessageWithRefrId<UpdateAnimationMessage> = {
                    t: MsgType.UpdateAnimation,
                    data: anim,
                    _refrId
                };
                this.controller.emitter.emit("sendMessageWithRefrId", {
                    message,
                    reliability: "unreliable"
                });
            }
        }
    }

    private sendAppearance(_refrId?: number) {
        if (_refrId) return;
        const shown = this.sp.Ui.isMenuOpen('RaceSex Menu');
        if (shown != this.isRaceSexMenuShown) {
            this.isRaceSexMenuShown = shown;
            if (!shown) {
                this.sp.printConsole('Exited from race menu');

                const appearance = getAppearance(this.sp.Game.getPlayer() as Actor);
                // TODO: log appearance contents to debug appearance issues?
                const message: MessageWithRefrId<UpdateAppearanceMessage> = {
                    t: MsgType.UpdateAppearance,
                    data: appearance,
                    _refrId
                };
                this.controller.emitter.emit("sendMessageWithRefrId", {
                    message,
                    reliability: "reliable"
                });
            }
        }
    }

    private sendEquipment(_refrId?: number) {
        if (_refrId) return;
        if (this.equipmentChanged) {
            this.equipmentChanged = false;

            ++this.numEquipmentChanges;

            const eq = getEquipment(
                this.sp.Game.getPlayer() as Actor,
                this.numEquipmentChanges,
            );
            const message: MessageWithRefrId<UpdateEquipmentMessage> = {
                t: MsgType.UpdateEquipment,
                data: eq,
                _refrId
            };
            this.controller.emitter.emit("sendMessageWithRefrId", {
                message,
                reliability: "reliable"
            });
        }
    }

    private sendHostAttempts() {
        const remoteId = nextHostAttempt();
        if (!remoteId) return;

        this.controller.emitter.emit("sendMessage", {
            message: {
                t: MsgType.Host,
                remoteId
            },
            reliability: "unreliable"
        });
    }

    private getInputOwner(_refrId?: number) {
        return _refrId
            ? this.sp.Actor.from(this.sp.Game.getFormEx(worldViewMisc.remoteIdToLocalId(_refrId)))
            : this.sp.Game.getPlayer();
    }

    private getForm(refrId: number | undefined, world: WorldModel): FormModel | undefined {
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

    private get singlePlayerService() {
        return this.controller.lookupListener(SinglePlayerService);
    }

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
