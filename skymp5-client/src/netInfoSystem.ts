import * as sp from "skyrimPlatform";
import { settings } from "skyrimPlatform";

export class NetInfo {
  static addReceivedPacketCount(count: number): void {
    this.receivedPacketCount += count;
  }

  static getAndClearReceivedPacketCount(): number {
    const value = this.receivedPacketCount;
    this.receivedPacketCount = 0;
    return value;
  }

  static addSentPacketCount(count: number): void {
    this.sentPacketCount += count;
  }

  static getAndClearSentPacketCount(): number {
    const value = this.sentPacketCount;
    this.sentPacketCount = 0;
    return value;
  }

  static setLocalLagUnits(distance: number): void {
    this.localLagUnits = distance;
  }

  static getLocalLagUnits() {
    return this.localLagUnits;
  }

  static isEnabled() {
    return !!settings["skymp5-client"]["show-net-info"];
  }

  private static receivedPacketCount = 0;
  private static sentPacketCount = 0;
  private static localLagUnits = 0;
}

class NetInfoTexts {
  public static readonly Name = "netInfoTexts";

  constructor(
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
    sp.destroyText(this.connectionStaticTextId);
    sp.destroyText(this.connectionStateTextId);
    sp.destroyText(this.receivedPacketStaticTextId);
    sp.destroyText(this.receivedPacketAmountTextId);
    sp.destroyText(this.sentPacketStaticTextId);
    sp.destroyText(this.sentPacketAmountTextId);
    sp.destroyText(this.localPositionLagStaticTextId);
    sp.destroyText(this.localPositionLagAmountTextId);
  }
}

if (sp.storage[NetInfoTexts.Name] && (sp.storage[NetInfoTexts.Name] as NetInfoTexts).clear) {
  (sp.storage[NetInfoTexts.Name] as NetInfoTexts)?.clear();
}
const delayMs: number = 1000;
let textIds: NetInfoTexts;
let lastDt: number = 0;
let dt: number = 0;

const greenARGB: number[] = [0, 128, 0, 1];
const redARGB: number[] = [255, 0, 0, 1];

export const start = (): void => {
  if (!NetInfo.isEnabled()) return;

  textIds = new NetInfoTexts();
  sp.storage[NetInfoTexts.Name] = textIds;
  lastDt = Date.now();

  sp.on("update", () => {
    dt += Date.now() - lastDt;
    lastDt = Date.now();

    const isConnected = sp.mpClientPlugin.isConnected();
    sp.setTextString(textIds.connectionStateTextId, `${isConnected ? "ON" : "OFF"}`);
    sp.setTextColor(textIds.connectionStateTextId, isConnected ? greenARGB : redARGB);

    // https://www.creationkit.com/index.php?title=Unit
    const units = NetInfo.getLocalLagUnits();
    const unitsInMeter = 70.0218818381;
    const meters = Math.round(units / unitsInMeter * 10) / 10;

    sp.setTextString(textIds.localPositionLagAmountTextId, `${units} units (~${meters} m)`);

    if (delayMs > dt) return;

    sp.setTextString(textIds.receivedPacketAmountTextId, `${Math.round(NetInfo.getAndClearReceivedPacketCount())}`);
    sp.setTextString(textIds.sentPacketAmountTextId, `${Math.round(NetInfo.getAndClearSentPacketCount())}`);
    dt = 0;
  });
}
