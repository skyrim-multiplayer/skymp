import {
  on,
  once,
  printConsole,
  storage,
  settings,
  Game,
  Ui,
} from "skyrimPlatform";
import { WorldView } from "./view";
import { getMovement } from "./components/movement";
import { getLook } from "./components/look";
import { AnimationSource, Animation, setupHooks } from "./components/animation";
import { getEquipment } from "./components/equipment";
import { getDiff, getInventory, Inventory } from "./components/inventory";
import { MsgType } from "./messages";
import { MsgHandler } from "./msgHandler";
import { ModelSource } from "./modelSource";
import { RemoteServer, getPcInventory } from "./remoteServer";
import { SendTarget } from "./sendTarget";
import * as networking from "./networking";
import * as sp from "skyrimPlatform";
import * as loadGameManager from "./loadGameManager";
import * as deathSystem from "./deathSystem";
import { count } from "console";

interface AnyMessage {
  type?: string;
  t?: number;
}
const handleMessage = (msgAny: AnyMessage, handler_: MsgHandler) => {
  const msgType: string = msgAny.type || MsgType[msgAny.t];
  const handler = (handler_ as unknown) as Record<
    string,
    (m: AnyMessage) => void
  >;
  const f = handler[msgType];
  if (msgType !== "UpdateMovement") printConsole(msgType, msgAny);
  if (f && typeof f === "function") handler[msgType](msgAny);
};

for (let i = 0; i < 100; ++i) printConsole();
printConsole("Hello Multiplayer");
printConsole("settings:", settings["skymp5-client"]);

const targetIp = settings["skymp5-client"]["server-ip"];
const targetPort = settings["skymp5-client"]["server-port"];

if (storage.targetIp !== targetIp || storage.targetPort !== targetPort) {
  storage.targetIp = targetIp;
  storage.targetPort = targetPort;

  printConsole(`Connecting to ${storage.targetIp}:${storage.targetPort}`);
  networking.connect(targetIp, targetPort);
} else {
  printConsole("Reconnect is not required");
}

export class SkympClient {
  constructor() {
    this.resetView();
    this.resetRemoteServer();
    setupHooks();

    sp.printConsole("SkympClient ctor");

    networking.on("connectionFailed", () => {
      printConsole("Connection failed");
    });

    networking.on("connectionDenied", (err: string) => {
      printConsole("Connection denied: ", err);
    });

    networking.on("connectionAccepted", () => {
      this.msgHandler.handleConnectionAccepted();
    });

    networking.on("disconnect", () => {
      this.msgHandler.handleDisconnect();
    });

    networking.on("message", (msgAny: Record<string, unknown>) => {
      handleMessage(msgAny, this.msgHandler);
    });

    on("update", () => {
      if (!this.singlePlayer) {
        this.sendInputs();
      }
    });

    let lastInv: Inventory;

    on("activate", (e) => {
      lastInv = getInventory(Game.getPlayer());
      const caster = e.caster ? e.caster.getFormID() : 0;
      const target = e.target ? e.target.getFormID() : 0;

      if (caster !== 0x14) return;

      if (!target || target >= 0xff000000) return;

      this.sendTarget.send(
        { t: MsgType.Activate, data: { caster, target } },
        true
      );
      printConsole("sendActi", { caster, target });
    });

    on("containerChanged", (e) => {
      if (e.oldContainer && e.newContainer) {
        if (
          e.oldContainer.getFormID() === 0x14 ||
          e.newContainer.getFormID() === 0x14
        ) {
          printConsole(1);
          if (!lastInv) lastInv = getPcInventory();
          if (lastInv) {
            printConsole(2);
            const newInv = getInventory(Game.getPlayer());
            const diff = getDiff(lastInv, newInv, true);
            printConsole({ diff });
            diff.entries.forEach((entry) => {
              if (entry.count !== 0) {
                const msg = JSON.parse(JSON.stringify(entry));
                delete msg["name"]; // Extra name works too strange
                msg["t"] = entry.count > 0 ? MsgType.PutItem : MsgType.TakeItem;
                msg["count"] = Math.abs(msg["count"]);
                msg["target"] =
                  e.oldContainer.getFormID() === 0x14
                    ? e.newContainer.getFormID()
                    : e.oldContainer.getFormID();
                this.sendTarget.send(msg, true);
              }
            });
            lastInv = newInv;
          }
        }
      }
    });

    const playerFormId = 0x14;
    on("equip", (e) => {
      if (e.actor.getFormID() === playerFormId) this.equipmentChanged = true;
    });
    on("unequip", (e) => {
      if (e.actor.getFormID() === playerFormId) this.equipmentChanged = true;
    });

    loadGameManager.addLoadGameListener((e: loadGameManager.GameLoadEvent) => {
      if (!e.isCausedBySkyrimPlatform && !this.singlePlayer) {
        sp.Debug.messageBox(
          "Save has been loaded in multiplayer, switching to the single-player mode"
        );
        networking.close();
        this.singlePlayer = true;
        Game.setInChargen(false, false, false);
      }
    });
    on("update", () => deathSystem.update());
  }

  private sendMovement() {
    const sendMovementRateMs = 130;
    const now = Date.now();
    if (now - this.lastSendMovementMoment > sendMovementRateMs) {
      this.sendTarget.send(
        { t: MsgType.UpdateMovement, data: getMovement(Game.getPlayer()) },
        false
      );
      this.lastSendMovementMoment = now;
    }
  }

  private sendAnimation() {
    if (!this.playerAnimSource) {
      this.playerAnimSource = new AnimationSource(Game.getPlayer());
    }
    const anim = this.playerAnimSource.getAnimation();
    if (
      !this.lastAnimationSent ||
      anim.numChanges !== this.lastAnimationSent.numChanges
    ) {
      if (anim.animEventName !== "") {
        this.lastAnimationSent = anim;
        this.sendTarget.send({ t: MsgType.UpdateAnimation, data: anim }, false);
      }
    }
  }

  private sendLook() {
    const shown = Ui.isMenuOpen("RaceSex Menu");
    if (shown != this.isRaceSexMenuShown) {
      this.isRaceSexMenuShown = shown;
      if (!shown) {
        printConsole("Exited from race menu");

        const look = getLook(Game.getPlayer());
        this.sendTarget.send({ t: MsgType.UpdateLook, data: look }, true);
      }
    }
  }

  private sendEquipment() {
    if (this.equipmentChanged) {
      this.equipmentChanged = false;

      ++this.numEquipmentChanges;

      const eq = getEquipment(Game.getPlayer(), this.numEquipmentChanges);
      this.sendTarget.send({ t: MsgType.UpdateEquipment, data: eq }, true);
      printConsole({ eq });
    }
  }

  private sendInputs() {
    this.sendMovement();
    this.sendAnimation();
    this.sendLook();
    this.sendEquipment();
  }

  private resetRemoteServer() {
    const prevRemoteServer: RemoteServer = storage.remoteServer;
    let rs: RemoteServer;

    if (prevRemoteServer && prevRemoteServer.getWorldModel) {
      rs = prevRemoteServer;
      printConsole("Restore previous RemoteServer");

      // Keep previous RemoteServer, but update func implementations
      const newObj: Record<
        string,
        unknown
      > = (new RemoteServer() as unknown) as Record<string, unknown>;
      const rsAny: Record<string, unknown> = (rs as unknown) as Record<
        string,
        unknown
      >;
      for (const key in newObj) {
        if (typeof newObj[key] === "function") rsAny[key] = newObj[key];
      }
    } else {
      rs = new RemoteServer();
      printConsole("Creating RemoteServer");
    }

    this.sendTarget = rs;
    this.msgHandler = rs;
    this.modelSource = rs;
    storage.remoteServer = rs;
  }

  private resetView() {
    const prevView: WorldView = storage.view;
    const view = new WorldView();
    once("update", () => {
      if (prevView && prevView.destroy) {
        prevView.destroy();
        printConsole("Previous View destroyed");
      }
      storage.view = view;
    });
    on("update", () => {
      if (!this.singlePlayer) view.update(this.modelSource.getWorldModel());
    });
  }

  private playerAnimSource?: AnimationSource;
  private lastSendMovementMoment = 0;
  private lastAnimationSent?: Animation;
  private msgHandler?: MsgHandler;
  private modelSource?: ModelSource;
  private sendTarget?: SendTarget;
  private isRaceSexMenuShown = false;
  private singlePlayer = false;
  private equipmentChanged = false;
  private numEquipmentChanges = 0;
}
