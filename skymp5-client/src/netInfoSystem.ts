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

const delayMs: number = 1000;
let last_dt: number = 0;
let dt: number = 0;

const greenARGB: number[] = [0, 128, 0, 1];
const redARGB: number[] = [255, 0, 0, 1];

let connectionStaticTextId = sp.createText(150, 350, "connection:", [255, 255, 255, 1]);
let connectionStateTextId = sp.createText(320, 350, "", [255, 255, 255, 1]);
let receivedPacketStaticTextId = sp.createText(188, 400, "incoming (p/s):", [255, 255, 255, 1]);
let receivedPacketAmountTextId = sp.createText(360, 400, "", [255, 255, 255, 1]);
let sentPacketStaticTextId = sp.createText(188, 450, "outgoing (p/s):", [255, 255, 255, 1]);
let sentPacketAmountTextId = sp.createText(360, 450, "", [255, 255, 255, 1]);

export const start = (): void => {
  last_dt = Date.now();
  sp.on("update", () => {
    dt += Date.now() - last_dt;
    last_dt = Date.now();

    const isConnected = sp.mpClientPlugin.isConnected();
    sp.setTextString(connectionStateTextId, `${isConnected ? "ON" : "OFF"}`);
    sp.setTextColor(connectionStateTextId, isConnected ? greenARGB : redARGB);

    if (delayMs > dt) return;

    sp.setTextString(receivedPacketAmountTextId, `${Math.round(NetInfo.getAndClearReceivedPacketAmount())}`);
    sp.setTextString(sentPacketAmountTextId, `${Math.round(NetInfo.getAndClearSentPacketAmount())}`);
    dt = 0;
  });
}
