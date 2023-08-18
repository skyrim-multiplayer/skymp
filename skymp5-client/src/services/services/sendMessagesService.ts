import { NeverError } from "../../lib/errors";
import { SendMessageEvent } from "../events/sendMessageEvent";
import { SendMessageWithRefrIdEvent } from "../events/sendMessageWithRefrIdEvent";
import { AnyMessage } from "../messages/anyMessage";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { SkympClient } from "./skympClient";

export class SendMessagesService extends ClientListener{
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        controller.emitter.on("sendMessage", (e) => this.onSendMessage(e));
        controller.emitter.on("sendMessageWithRefrId", (e) => this.onSendMessageWithRefrId(e));
    }

    private onSendMessage(e: SendMessageEvent<AnyMessage>) {
        const skympClient = this.controller.lookupListener(SkympClient);
        skympClient.sendTarget?.send(e.message as any, this.isReliable(e.reliability));
    }

    private onSendMessageWithRefrId(e: SendMessageWithRefrIdEvent<AnyMessage>) {
        // Right now sendTarget.send itself handles _refrId
        // So the code is the same as for onSendMessage
        const skympClient = this.controller.lookupListener(SkympClient);
        skympClient.sendTarget?.send(e.message as any, this.isReliable(e.reliability));
    }

    private isReliable(reliability: "reliable" | "unreliable") {
        switch (reliability) {
            case "reliable":
                return true;
            case "unreliable":
                return false;
            default:
                throw new NeverError(reliability);
        }
    }
}
