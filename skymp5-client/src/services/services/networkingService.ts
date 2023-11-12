import { NetInfo } from "../../debug/netInfoSystem";
import { NeverError } from "../../lib/errors";
import { MsgType } from "../../messages";
import { SendMessageEvent } from "../events/sendMessageEvent";
import { SendMessageWithRefrIdEvent } from "../events/sendMessageWithRefrIdEvent";
import { AnyMessage } from "../messages/anyMessage";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { RemoteServer } from "./remoteServer";
import { SkympClient } from "./skympClient";

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
          }
          else {
            if (msgAny.t === MsgType.Activate) {
              
            }
          }

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
          this.controller.emitter.on("hostStartMessage", (e) => this.onHostStartMessage(e));
          this.controller.emitter.on("hostStopMessage", (e) => this.onHostStopMessage(e));
          this.controller.emitter.on("setInventoryMessage", (e) => this.onSetInventoryMessage(e));
          this.controller.emitter.on("createActorMessage", (e) => this.onCreateActorMessage(e));
          this.controller.emitter.on("customPacketMessage2", (e) => this.onCustomPacketMessage2(e));
          this.controller.emitter.on("destroyActorMessage", (e) => this.onDestroyActorMessage(e));
          this.controller.emitter.on("setRaceMenuOpenMessage", (e) => this.onSetRaceMenuOpenMessage(e));
          this.controller.emitter.on("spSnippetMessage", (e) => this.onSpSnippetMessage(e));
          this.controller.emitter.on("updateGamemodeDataMessage", (e) => this.onUpdateGamemodeDataMessage(e));
          this.controller.emitter.on("updatePropertyMessage", (e) => this.onUpdatePropertyMessage(e));
          this.controller.emitter.on("deathStateContainerMessage", (e) => this.onDeathStateContainerMessage(e));

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
