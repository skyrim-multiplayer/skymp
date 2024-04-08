import { logTrace, logError } from "../../logging";
import { NeverError } from "../../lib/errors";
import { MsgType } from "../../messages";
import { SendMessageEvent } from "../events/sendMessageEvent";
import { SendMessageWithRefrIdEvent } from "../events/sendMessageWithRefrIdEvent";
import { AnyMessage } from "../messages/anyMessage";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { RemoteServer } from "./remoteServer";

export class NetworkingService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    this.controller.on("tick", () => this.onTick());

    this.controller.emitter.on("sendMessage", (e) => this.onSendMessage(e));
    this.controller.emitter.on("sendMessageWithRefrId", (e) => this.onSendMessageWithRefrId(e));
  }

  private onSendMessage(e: SendMessageEvent<AnyMessage>) {
    this.sp.mpClientPlugin.send(JSON.stringify(e.message), this.isReliable(e.reliability));
  }

  private onSendMessageWithRefrId(e: SendMessageWithRefrIdEvent<AnyMessage>) {
    const refrId = e.message._refrId;

    const remoteServer = this.controller.lookupListener(RemoteServer);

    const idxInModel = refrId
      ? remoteServer.getWorldModel().forms.findIndex((f) => f && f.refrId === refrId)
      : remoteServer.getWorldModel().playerCharacterFormIdx;

    // fixes "can't get property idx of null or undefined"
    if (!remoteServer.getWorldModel().forms[idxInModel]) return;

    // @ts-ignore
    e.message.idx = remoteServer.getWorldModel().forms[idxInModel].idx;

    delete e.message._refrId;

    this.sp.mpClientPlugin.send(JSON.stringify(e.message), this.isReliable(e.reliability));
  }

  connect(hostName: string, port: number) {
    this.serverAddress = { hostName, port };
    this.createClientSafe();
  }

  reconnect() {
    this.createClientSafe();
  }

  close() {
    this.sp.mpClientPlugin.destroyClient();
  }

  send(msg: Record<string, unknown>, reliable: boolean) {
    // TODO(#175): JS object instead of JSON?
    this.sp.mpClientPlugin.send(JSON.stringify(msg), reliable);
  }

  private onTick() {
    this.sp.mpClientPlugin.tick((packetType, jsonContent, error) => {
      switch (packetType) {
        case "connectionAccepted":
          this.controller.emitter.emit("connectionAccepted", {});
          break;
        case "connectionDenied":
          this.controller.emitter.emit("connectionDenied", { error });
          this.reconnect();
          break;
        case "connectionFailed":
          this.controller.emitter.emit("connectionFailed", {});
          this.reconnect();
          break;
        case "disconnect":
          this.controller.emitter.emit("connectionDisconnect", {});
          this.reconnect();
          break;
        case "message":
          // TODO: in theory can be empty jsonContent and non-empty error
          const msgAny: AnyMessage = JSON.parse(jsonContent);

          if ("type" in msgAny) {
            if (msgAny.type === "createActor") {
              const event = { message: msgAny };
              this.controller.emitter.emit("createActorMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.type === "customPacket") {
              const event = { message: msgAny };
              this.controller.emitter.emit("customPacketMessage2", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.type === "destroyActor") {
              const event = { message: msgAny };
              this.controller.emitter.emit("destroyActorMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.type === "hostStart") {
              const event = { message: msgAny };
              this.controller.emitter.emit("hostStartMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.type === "hostStop") {
              const event = { message: msgAny };
              this.controller.emitter.emit("hostStopMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.type === "setInventory") {
              const event = { message: msgAny };
              this.controller.emitter.emit("setInventoryMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.type === "setRaceMenuOpen") {
              const event = { message: msgAny };
              this.controller.emitter.emit("setRaceMenuOpenMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.type === "spSnippet") {
              const event = { message: msgAny };
              this.controller.emitter.emit("spSnippetMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.type === "updateGamemodeData") {
              const event = { message: msgAny };
              this.controller.emitter.emit("updateGamemodeDataMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.type === "teleport") {
              const event = { message: msgAny };
              this.controller.emitter.emit("teleportMessage2", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else {
              throw new NeverError(msgAny);
            }
          }
          else {
            if (msgAny.t === MsgType.OpenContainer) {
              const event = { message: msgAny };
              this.controller.emitter.emit("openContainerMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.t === MsgType.UpdateMovement) {
              const event = { message: msgAny };
              this.controller.emitter.emit("updateMovementMessage", event)
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.t === MsgType.UpdateAnimation) {
              const event = { message: msgAny };
              this.controller.emitter.emit("updateAnimationMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.t === MsgType.UpdateEquipment) {
              const event = { message: msgAny };
              this.controller.emitter.emit("updateEquipmentMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.t === MsgType.ChangeValues) {
              const event = { message: msgAny };
              this.controller.emitter.emit("changeValuesMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.t === MsgType.UpdateAppearance) {
              const event = { message: msgAny };
              this.controller.emitter.emit("updateAppearanceMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.t === MsgType.Teleport) {
              const event = { message: msgAny };
              this.controller.emitter.emit("teleportMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.t === MsgType.UpdateProperty) {
              const event = { message: msgAny };
              this.controller.emitter.emit("updatePropertyMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            else if (msgAny.t === MsgType.DeathStateContainer) {
              const event = { message: msgAny };
              this.controller.emitter.emit("deathStateContainerMessage", event);
              this.controller.emitter.emit("anyMessage", event);
            }
            // todo: never error
          }
          break;
      }
    });
  }

  private createClientSafe() {
    const { hostName, port } = this.serverAddress;

    logTrace(this, "createClientSafe " + hostName + ":" + port);

    if (this.serverAddress.hostName !== "" && this.serverAddress.port !== 0) {
      this.sp.mpClientPlugin.createClient(hostName, port);
    }
    else {
      logError(this, "createClientSafe failed");
    }
  }

  private get serverAddress(): { hostName: string, port: number } {
    const res: unknown = this.sp.storage["serverAddress"];
    if (typeof res === "object") {
      const result = res as { hostName: string, port: number };
      if (typeof result.hostName === "string" && typeof result.port === "number") {
        return result;
      }
    }
    return { hostName: "", port: 0 };
  }

  private set serverAddress(newValue: { hostName: string, port: number }) {
    this.sp.storage["serverAddress"] = newValue;
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
};
