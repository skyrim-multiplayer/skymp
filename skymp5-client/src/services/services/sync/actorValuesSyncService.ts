import { MessageWithRefrId, SendMessageWithRefrIdEvent } from "../../../services/events/sendMessageWithRefrIdEvent";
import { ClientListener, Sp, CombinedController } from "../clientListener";
import { AnyMessage } from "../../../services/messages/anyMessage";
import { MsgType } from "../../../messages";
import { UpdateAnimationMessage } from "../../../services/messages/updateAnimationMessage";
import { ActorValues, getActorValues } from "../../../sync/actorvalues";
import { RemoteServer } from "../remoteServer";
import { Actor } from "skyrimPlatform";
import { logTrace } from "../../../logging";
import { ChangeValuesMessage } from "../../../services/messages/changeValuesMessage";
import { DeathService } from "../deathService";
import { SendInputsService } from "../sendInputsService";

export class ActorValuesSyncService extends ClientListener {

    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        this.controller.on("update", () => this.onUpdate());
        this.controller.emitter.on("sendMessageWithRefrId", (e) => this.onSendMessageWithRefrId(e));
    }

    private onUpdate() {
        this.sendActorValuePercentage(undefined);
    }

    private sendActorValuePercentage(_refrId: number | undefined) {
        const remoteServer = this.controller.lookupListener(RemoteServer);
        const sendInputsService = this.controller.lookupListener(SendInputsService); // TODO: refactor out

        const worldModel = remoteServer.getWorldModel();
        const form = sendInputsService.getForm(_refrId, worldModel);
        const canSend = form && (form.isDead ?? false) === false;
        if (!canSend) {
            return;
        }

        const owner = sendInputsService.getInputOwner(_refrId);
        if (!owner) {
            return;
        }

        const av = getActorValues(this.sp.Game.getPlayer() as Actor);
        const currentTime = Date.now();
        if (
            this.actorValuesNeedUpdate === false &&
            this.prevValues.health === av.health &&
            this.prevValues.stamina === av.stamina &&
            this.prevValues.magicka === av.magicka
        ) {
            return;
        }


        if (
            currentTime - this.prevActorValuesUpdateTime < 2000 &&
            this.actorValuesNeedUpdate === false
        ) {
            return;
        }

        const deathService = this.controller.lookupListener(DeathService);
        if (deathService.isBusy()) {
            logTrace(this, "Not sending actor values, death service is busy");
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

    private onSendMessageWithRefrId(event: SendMessageWithRefrIdEvent<AnyMessage>): void {
        const msg = event.message;

        if (msg.t !== MsgType.UpdateAnimation) {
            return;
        }

        const typedMessage = msg as MessageWithRefrId<UpdateAnimationMessage>;

        if (
            typedMessage.data.animEventName !== 'JumpLand' &&
            typedMessage.data.animEventName !== 'JumpLandDirectional' &&
            typedMessage.data.animEventName !== 'DeathAnim'
        ) {
            this.actorValuesNeedUpdate = true;
        }
    }

    // Not adapted for multi-actor yet
    private actorValuesNeedUpdate = false;
    private prevValues: ActorValues = { health: 0, stamina: 0, magicka: 0 };
    private prevActorValuesUpdateTime = 0;
};
