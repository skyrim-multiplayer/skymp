import { NewLocalLagValueCalculatedEvent } from "../events/newLocalLagValueCalculatedEvent";
import { ConnectionMessage } from "../events/connectionMessage";
import { SendMessageEvent } from "../events/sendMessageEvent";
import { SendMessageWithRefrIdEvent } from "../events/sendMessageWithRefrIdEvent";
import { AnyMessage } from "../messages/anyMessage";
import { ClientListener, CombinedController, Sp } from "./clientListener";

export class NetInfoService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    // clear previous texts in case of hotreload
    if (this.sp.storage[NetInfoTexts.Name] && (this.sp.storage[NetInfoTexts.Name] as NetInfoTexts).clear) {
      this.logTrace(`Destroying old NetInfoTexts`);
      try {
        (this.sp.storage[NetInfoTexts.Name] as NetInfoTexts)?.clear();
      }
      catch (e) {
        this.logError(`Failed to destroy old NetInfoTexts: ${e}`);
      }
    }

    if (!this.isEnabled()) return;

    this.textIds = new NetInfoTexts(this.sp);
    this.sp.storage[NetInfoTexts.Name] = this.textIds;
    this.lastDt = Date.now();

    this.controller.emitter.on("sendMessage", (e) => this.onSendMessage(e));
    this.controller.emitter.on("sendMessageWithRefrId", (e) => this.onSendMessageWithRefrId(e));
    this.controller.emitter.on("anyMessage", (e) => this.onAnyMessage(e));
    this.controller.emitter.on("newLocalLagValueCalculated", (e) => this.onNewLocalLagValueCalculated(e));
    this.controller.on("update", () => this.onUpdate());
  }

  private onSendMessage(e: SendMessageEvent<AnyMessage>) {
    this.addSentPacketCount(1);
  }

  private onSendMessageWithRefrId(e: SendMessageWithRefrIdEvent<AnyMessage>) {
    this.addSentPacketCount(1);
  }

  private onAnyMessage(e: ConnectionMessage<AnyMessage>) {
    this.addReceivedPacketCount(1);
  }

  private onNewLocalLagValueCalculated(e: NewLocalLagValueCalculatedEvent) {
    this.setLocalLagUnits(e.lagUnitsNoZ);
  }

  private onUpdate() {
    if (this.textIds === undefined) return;

    this.dt += Date.now() - this.lastDt;
    this.lastDt = Date.now();

    const isConnected = this.sp.mpClientPlugin.isConnected();
    this.sp.setTextString(this.textIds.connectionStateTextId, `${isConnected ? "ON" : "OFF"}`);
    this.sp.setTextColor(this.textIds.connectionStateTextId, isConnected ? this.greenARGB : this.redARGB);

    // https://www.creationkit.com/index.php?title=Unit
    const units = this.getLocalLagUnits();
    const unitsInMeter = 70.0218818381;
    const meters = Math.round(units / unitsInMeter * 10) / 10;

    this.sp.setTextString(this.textIds.localPositionLagAmountTextId, `${units} units (~${meters} m)`);

    if (this.delayMs > this.dt) return;

    this.sp.setTextString(this.textIds.receivedPacketAmountTextId, `${Math.round(this.getAndClearReceivedPacketCount())}`);
    this.sp.setTextString(this.textIds.sentPacketAmountTextId, `${Math.round(this.getAndClearSentPacketCount())}`);
    this.dt = 0;
  }

  private addReceivedPacketCount(count: number): void {
    this.receivedPacketCount += count;
  }

  private getAndClearReceivedPacketCount(): number {
    const value = this.receivedPacketCount;
    this.receivedPacketCount = 0;
    return value;
  }

  private addSentPacketCount(count: number): void {
    this.sentPacketCount += count;
  }

  private getAndClearSentPacketCount(): number {
    const value = this.sentPacketCount;
    this.sentPacketCount = 0;
    return value;
  }

  private setLocalLagUnits(distance: number): void {
    this.localLagUnits = distance;
  }

  private getLocalLagUnits() {
    return this.localLagUnits;
  }

  private isEnabled() {
    return !!this.sp.settings["skymp5-client"]["show-net-info"];
  }

  private receivedPacketCount = 0;
  private sentPacketCount = 0;
  private localLagUnits = 0;

  private delayMs = 1000;
  private textIds?: NetInfoTexts;
  private lastDt = 0;
  private dt = 0;

  private readonly greenARGB = [0, 128, 0, 1];
  private readonly redARGB = [255, 0, 0, 1];
}

class NetInfoTexts {
  public static readonly Name = "netInfoTexts";

  constructor(
    private readonly sp: Sp,
    public readonly connectionStaticTextId = sp.createText(100, 350, "connection:", [255, 255, 255, 1]),
    public readonly connectionStateTextId = sp.createText(220, 350, "", [255, 255, 255, 1]),
    public readonly receivedPacketStaticTextId = sp.createText(120, 390, "incoming (p/s):", [255, 255, 255, 1]),
    public readonly receivedPacketAmountTextId = sp.createText(250, 390, "", [255, 255, 255, 1]),
    public readonly sentPacketStaticTextId = sp.createText(120, 430, "outgoing (p/s):", [255, 255, 255, 1]),
    public readonly sentPacketAmountTextId = sp.createText(250, 430, "", [255, 255, 255, 1]),
    public readonly localPositionLagStaticTextId = sp.createText(90, 470, "local lag:", [255, 255, 255, 1]),
    public readonly localPositionLagAmountTextId = sp.createText(250, 470, "", [255, 255, 255, 1]),
  ) { }

  public clear(): void {
    this.sp.destroyText(this.connectionStaticTextId);
    this.sp.destroyText(this.connectionStateTextId);
    this.sp.destroyText(this.receivedPacketStaticTextId);
    this.sp.destroyText(this.receivedPacketAmountTextId);
    this.sp.destroyText(this.sentPacketStaticTextId);
    this.sp.destroyText(this.sentPacketAmountTextId);
    this.sp.destroyText(this.localPositionLagStaticTextId);
    this.sp.destroyText(this.localPositionLagAmountTextId);
  }
}
