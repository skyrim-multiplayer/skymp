/* eslint-disable @typescript-eslint/no-empty-function */
import * as networking from "./networking";
import { FormModel, WorldModel } from "./model";
import { MsgHandler } from "./msgHandler";
import { ModelSource } from "./modelSource";
import { SendTarget } from "./sendTarget";
import * as messages from "./messages";
import {
  Game,
  once,
  TESModPlatform,
  Cell,
  WorldSpace,
  printConsole,
  Utility,
  writePlugin,
  storage,
  getPluginSourceCode,
  browser,
  ObjectReference,
  on,
  Ui,
} from "skyrimPlatform";
import * as loadGameManager from "./loadGameManager";
import { applyInventory, Inventory } from "./components/inventory";
import { isBadMenuShown } from "./components/equipment";
import { Movement } from "./components/movement";
import { IdManager } from "../lib/idManager";

class SpawnTask {
  running = false;
}

const sendBrowserToken = () => {
  networking.send(
    {
      t: messages.MsgType.CustomPacket,
      content: {
        customPacketType: "browserToken",
        token: browser.getToken(),
      },
    },
    true
  );
};

const verifySourceCode = () => {
  const src = getPluginSourceCode("skymp5-client");
  printConsole(`Verifying current source code (${src.length} bytes)`);
  networking.send(
    {
      t: messages.MsgType.CustomPacket,
      content: { customPacketType: "clientVersion", src },
    },
    true
  );
};

const taskVerifySourceCode = () => {
  storage["taskVerifySourceCode"] = true;
};

if (storage["taskVerifySourceCode"] === true) {
  once("tick", () => {
    verifySourceCode();
  });
  storage["taskVerifySourceCode"] = false;
}

export const getPcInventory = (): Inventory => {
  const res = storage["pcInv"];
  if (typeof res === "object" && res["entries"]) {
    return res;
  }
  return null;
};

const setPcInventory = (inv: Inventory): void => {
  storage["pcInv"] = inv;
};

let pcInvLastApply = 0;
on("update", () => {
  if (Date.now() - pcInvLastApply > 5000 && !isBadMenuShown()) {
    pcInvLastApply = Date.now();
    const pcInv = getPcInventory();
    if (pcInv) applyInventory(Game.getPlayer(), pcInv, false, true);
  }
});

export class RemoteServer implements MsgHandler, ModelSource, SendTarget {
  setInventory(msg: messages.SetInventory): void {
    once("update", () => {
      setPcInventory(msg.inventory);
      pcInvLastApply = 0;
    });
  }

  openContainer(msg: messages.OpenContainer): void {
    once("update", async () => {
      await Utility.wait(0.1); // Give a chance to update inventory
      ObjectReference.from(Game.getFormEx(msg.target)).activate(
        Game.getPlayer(),
        true
      );
      (async () => {
        while (!Ui.isMenuOpen("ContainerMenu")) await Utility.wait(0.1);
        while (Ui.isMenuOpen("ContainerMenu")) await Utility.wait(0.1);
        networking.send(
          {
            t: messages.MsgType.Activate,
            data: { caster: 0x14, target: msg.target },
          },
          true
        );
      })();
    });
  }

  teleport(msg: messages.Teleport): void {
    once("update", () => {
      TESModPlatform.moveRefrToPosition(
        Game.getPlayer(),
        Cell.from(Game.getFormEx(msg.worldOrCell)),
        WorldSpace.from(Game.getFormEx(msg.worldOrCell)),
        msg.pos[0],
        msg.pos[1],
        msg.pos[2],
        msg.rot[0],
        msg.rot[1],
        msg.rot[2]
      );
      Utility.wait(0.2).then(() => {
        Game.getPlayer().setAngle(msg.rot[0], msg.rot[1], msg.rot[2]);
      });
    });
  }

  createActor(msg: messages.CreateActorMessage): void {
    const i = this.getIdManager().allocateIdFor(msg.idx);
    if (this.worldModel.forms.length <= i) this.worldModel.forms.length = i + 1;

    let movement: Movement = null;
    if (!msg.refrId) {
      movement = {
        pos: msg.transform.pos,
        rot: msg.transform.rot,
        worldOrCell: msg.transform.worldOrCell,
        runMode: "Standing",
        direction: 0,
        isInJumpState: false,
        isSneaking: false,
        isBlocking: false,
        isWeapDrawn: false,
        healthPercentage: 1.0,
      };
    }

    this.worldModel.forms[i] = {
      movement,
      numMovementChanges: 0,
      numLookChanges: 0,
      baseId: msg.baseId,
      refrId: msg.refrId,
    };
    if (msg.look) {
      this.worldModel.forms[i].look = msg.look;
    }

    if (msg.equipment) {
      this.worldModel.forms[i].equipment = msg.equipment;
    }

    if (msg.props) {
      for (const propName in msg.props) {
        this.UpdateProperty({
          t: messages.MsgType.UpdateProperty,
          propName,
          data: msg.props[propName],
          idx: i,
        });
      }
    }

    if (msg.isMe) this.worldModel.playerCharacterFormIdx = i;

    // TODO: move to a separate module

    const applyPcInv = () => {
      applyInventory(
        Game.getPlayer(),
        msg.inventory ? msg.inventory : { entries: [] },
        false
      );
      if (msg.inventory)
        this.setInventory({ type: "setInventory", inventory: msg.inventory });
    };

    if (msg.isMe) {
      const task = new SpawnTask();
      once("update", () => {
        if (!task.running) {
          task.running = true;
          printConsole("Using moveRefrToPosition to spawn player");
          TESModPlatform.moveRefrToPosition(
            Game.getPlayer(),
            Cell.from(Game.getFormEx(msg.transform.worldOrCell)),
            WorldSpace.from(Game.getFormEx(msg.transform.worldOrCell)),
            msg.transform.pos[0],
            msg.transform.pos[1],
            msg.transform.pos[2],
            msg.transform.rot[0],
            msg.transform.rot[1],
            msg.transform.rot[2]
          );
          // Unfortunatelly it requires two calls to work
          Utility.wait(1).then(applyPcInv);
          Utility.wait(1.3).then(applyPcInv);
        }
      });
      once("tick", () => {
        once("tick", () => {
          if (!task.running) {
            task.running = true;
            printConsole("Using loadGame to spawn player");
            loadGameManager.loadGame(
              msg.transform.pos,
              msg.transform.rot,
              msg.transform.worldOrCell
            );
            once("update", () => {
              applyPcInv();
              Utility.wait(0.3).then(applyPcInv);
            });
          }
        });
      });
    }
  }

  destroyActor(msg: messages.DestroyActorMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i] = null;

    // Shrink to fit
    while (1) {
      const length = this.worldModel.forms.length;
      if (!length) break;
      if (this.worldModel.forms[length - 1]) break;
      this.worldModel.forms.length = length - 1;
    }

    if (this.worldModel.playerCharacterFormIdx === i) {
      this.worldModel.playerCharacterFormIdx = -1;

      // TODO: move to a separate module
      Game.quitToMainMenu();
    }

    this.getIdManager().freeIdFor(msg.idx);
  }

  UpdateMovement(msg: messages.UpdateMovementMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].movement = msg.data;
    if (!this.worldModel.forms[i].numMovementChanges) {
      this.worldModel.forms[i].numMovementChanges = 0;
    }
    this.worldModel.forms[i].numMovementChanges++;
  }

  UpdateAnimation(msg: messages.UpdateAnimationMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].animation = msg.data;
  }

  UpdateLook(msg: messages.UpdateLookMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].look = msg.data;
    if (!this.worldModel.forms[i].numLookChanges) {
      this.worldModel.forms[i].numLookChanges = 0;
    }
    this.worldModel.forms[i].numLookChanges++;
  }

  UpdateEquipment(msg: messages.UpdateEquipmentMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    this.worldModel.forms[i].equipment = msg.data;
  }

  UpdateProperty(msg: messages.UpdatePropertyMessage): void {
    const i = this.getIdManager().getId(msg.idx);
    (this.worldModel.forms[i] as Record<string, unknown>)[msg.propName] =
      msg.data;
  }

  handleConnectionAccepted(): void {
    this.worldModel.forms = [];
    this.worldModel.playerCharacterFormIdx = -1;

    verifySourceCode();
    sendBrowserToken();
  }

  handleDisconnect(): void {}

  setRaceMenuOpen(msg: messages.SetRaceMenuOpenMessage): void {
    if (msg.open) {
      // wait 0.3s cause we can see visual bugs when teleporting
      // and showing this menu at the same time in onConnect
      once("update", () => Utility.wait(0.3).then(() => Game.showRaceMenu()));
    } else {
      // TODO: Implement closeMenu in SkyrimPlatform
    }
  }

  customPacket(msg: messages.CustomPacket): void {
    switch (msg.content.customPacketType) {
      case "newClientVersion":
        if (typeof msg.content.src !== "string")
          throw new Error(`'${msg.content.src}' is not a string`);
        const src: string = msg.content.src as string;

        // Force reconnecting after hot reload (see skympClient.ts)
        //networking.close();
        //storage.targetIp = "";

        taskVerifySourceCode();

        printConsole(`writing new version (${src} bytes)`);
        if (src.length > 0) writePlugin("skymp5-client", src);
        break;
    }
  }

  getWorldModel(): WorldModel {
    return this.worldModel;
  }

  getMyActorIndex(): number {
    return this.worldModel.playerCharacterFormIdx;
  }

  send(msg: Record<string, unknown>, reliable: boolean): void {
    if (this.worldModel.playerCharacterFormIdx === -1) return;

    msg.idx = this.getIdManager().getValueById(
      this.worldModel.playerCharacterFormIdx
    );
    networking.send(msg, reliable);
  }

  private getIdManager() {
    if (!this.idManager_) this.idManager_ = new IdManager();
    return this.idManager_;
  }

  private worldModel: WorldModel = { forms: [], playerCharacterFormIdx: -1 };
  private idManager_ = new IdManager();
}
