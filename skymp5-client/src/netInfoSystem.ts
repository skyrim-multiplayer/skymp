import * as sp from "skyrimPlatform";

export class NetInfo {
  private static _receivedPacketAmount: number = 0;
  private static _sentPacketAmount: number = 0;

  public static addReceivedPacketAmount(amount: number): void {
    this._receivedPacketAmount += amount;
  }

  public static getAndClearReceivedPacketAmount(): number {
    const value = this._receivedPacketAmount;
    this._receivedPacketAmount = 0;
    return value;
  }

  public static addSentPacketAmount(amount: number): void {
    this._sentPacketAmount += amount;
  }

  public static getAndClearSentPacketAmount(): number {
    const value = this._sentPacketAmount;
    this._sentPacketAmount = 0;
    return value;
  }
}

class netInfoTexts {
  public static readonly Name = "netInfoTexts";

  constructor(
    public readonly connectionStaticTextId = sp.createText(100, 350, "connection:", [255, 255, 255, 1]),
    public readonly connectionStateTextId = sp.createText(220, 350, "", [255, 255, 255, 1]),
    public readonly receivedPacketStaticTextId = sp.createText(120, 390, "incoming (p/s):", [255, 255, 255, 1]),
    public readonly receivedPacketAmountTextId = sp.createText(250, 390, "", [255, 255, 255, 1]),
    public readonly sentPacketStaticTextId = sp.createText(120, 430, "outgoing (p/s):", [255, 255, 255, 1]),
    public readonly sentPacketAmountTextId = sp.createText(250, 430, "", [255, 255, 255, 1]),
  ) { }

  public clear(): void {
    sp.destroyText(this.connectionStaticTextId);
    sp.destroyText(this.connectionStateTextId);
    sp.destroyText(this.receivedPacketStaticTextId);
    sp.destroyText(this.receivedPacketAmountTextId);
    sp.destroyText(this.sentPacketStaticTextId);
    sp.destroyText(this.sentPacketAmountTextId);
  }
}

if (sp.storage[netInfoTexts.Name] && (sp.storage[netInfoTexts.Name] as netInfoTexts).clear) {
  (sp.storage[netInfoTexts.Name] as netInfoTexts)?.clear();
}
const delayMs: number = 1000;
let textIds: netInfoTexts;
let last_dt: number = 0;
let dt: number = 0;

const greenARGB: number[] = [0, 128, 0, 1];
const redARGB: number[] = [255, 0, 0, 1];

export const start = (): void => {
  textIds = new netInfoTexts();
  sp.storage[netInfoTexts.Name] = textIds;
  last_dt = Date.now();

  sp.on("update", () => {
    dt += Date.now() - last_dt;
    last_dt = Date.now();

    const isConnected = sp.mpClientPlugin.isConnected();
    sp.setTextString(textIds.connectionStateTextId, `${isConnected ? "ON" : "OFF"}`);
    sp.setTextColor(textIds.connectionStateTextId, isConnected ? greenARGB : redARGB);

    if (delayMs > dt) return;

    sp.setTextString(textIds.receivedPacketAmountTextId, `${Math.round(NetInfo.getAndClearReceivedPacketAmount())}`);
    sp.setTextString(textIds.sentPacketAmountTextId, `${Math.round(NetInfo.getAndClearSentPacketAmount())}`);
    dt = 0;
  });
}
