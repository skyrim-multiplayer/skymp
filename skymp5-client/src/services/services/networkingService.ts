import { NetInfo } from "../../debug/netInfoSystem";
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

    // TODO: NetInfo should subscribe itself instead of incrementing here
    NetInfo.addSentPacketCount(1);

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
          // TODO: NetInfo should subscribe that event instead of calling this method here
          NetInfo.addReceivedPacketCount(1);

          // TODO: in theory can be empty jsonContent and non-empty error
          const msgAny: AnyMessage = JSON.parse(jsonContent);

          if ("type" in msgAny) {
            if (msgAny.type === "createActor") {
              this.controller.emitter.emit("createActorMessage", { message: msgAny })
            }
            else if (msgAny.type === "customPacket") {
              this.controller.emitter.emit("customPacketMessage2", { message: msgAny })
            }
            else if (msgAny.type === "destroyActor") {
              this.controller.emitter.emit("destroyActorMessage", { message: msgAny })
            }
            else if (msgAny.type === "hostStart") {
              this.controller.emitter.emit("hostStartMessage", { message: msgAny });
            }
            else if (msgAny.type === "hostStop") {
              this.controller.emitter.emit("hostStopMessage", { message: msgAny });
            }
            else if (msgAny.type === "setInventory") {
              this.controller.emitter.emit("setInventoryMessage", { message: msgAny });
            }
            else if (msgAny.type === "setRaceMenuOpen") {
              this.controller.emitter.emit("setRaceMenuOpenMessage", { message: msgAny });
            }
            else if (msgAny.type === "spSnippet") {
              this.controller.emitter.emit("spSnippetMessage", { message: msgAny });
            }
            else if (msgAny.type === "updateGamemodeData") {
              this.controller.emitter.emit("updateGamemodeDataMessage", { message: msgAny });
            }
            else {
              throw new NeverError(msgAny);
            }
          }
          else {
            if (msgAny.t === MsgType.OpenContainer) {
              this.controller.emitter.emit("openContainerMessage", { message: msgAny });
            }
            else if (msgAny.t === MsgType.UpdateMovement) {
              this.controller.emitter.emit("updateMovementMessage", { message: msgAny })
            }
            else if (msgAny.t === MsgType.UpdateAnimation) {
              this.controller.emitter.emit("updateAnimationMessage", { message: msgAny });
            }
            else if (msgAny.t === MsgType.UpdateEquipment) {
              this.controller.emitter.emit("updateEquipmentMessage", { message: msgAny });
            }
            else if (msgAny.t === MsgType.ChangeValues) {
              this.controller.emitter.emit("changeValuesMessage", { message: msgAny });
            }
            else if (msgAny.t === MsgType.UpdateAppearance) {
              this.controller.emitter.emit("updateAppearanceMessage", { message: msgAny });
            }
            else if (msgAny.t === MsgType.Teleport) {
              this.controller.emitter.emit("teleportMessage", { message: msgAny });
            }
            else if (msgAny.t === MsgType.UpdateProperty) {
              this.controller.emitter.emit("updatePropertyMessage", { message: msgAny });
            }
            else if (msgAny.t === MsgType.DeathStateContainer) {
              this.controller.emitter.emit("deathStateContainerMessage", { message: msgAny });
            }
            // todo: never error
          }
          break;
      }
    });
  }

  private createClientSafe() {
    const { hostName, port } = this.serverAddress;

    this.logTrace("createClientSafe " + hostName + ":" + port);

    if (this.serverAddress.hostName !== "" && this.serverAddress.port !== 0) {
      this.sp.mpClientPlugin.createClient(hostName, port);
    }
    else {
      this.logError("createClientSafe failed");
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
