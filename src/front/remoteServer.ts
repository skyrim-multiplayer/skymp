/* eslint-disable @typescript-eslint/no-empty-function */
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
} from "@skymp/skyrim-platform";

import * as networking from "./networking";
import { FormModel, WorldModel } from "./model";
import { MsgHandler } from "./msgHandler";
import { ModelSource } from "./modelSource";
import { SendTarget } from "./sendTarget";
import * as messages from "./messages";
import * as loadGameManager from "./loadGameManager";

interface FormModelInfo extends FormModel {
  dummy: undefined;
}

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

export class RemoteServer implements MsgHandler, ModelSource, SendTarget {
  createActor(msg: messages.CreateActorMessage): void {
    const i = msg.idx;
    if (this.forms.length <= i) this.forms.length = i + 1;

    this.forms[i] = {
      dummy: undefined,
      movement: {
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
      },
    };
    if (msg.look) {
      this.forms[i].look = msg.look;
    }

    if (msg.equipment) {
      this.forms[i].equipment = msg.equipment;
    }

    if (msg.isMe) this.myActorIndex = i;

    // TODO: move to a separate module
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
          }
        });
      });
    }
  }

  destroyActor(msg: messages.DestroyActorMessage): void {
    const i = msg.idx;
    this.forms[i] = null;

    if (this.myActorIndex === msg.idx) {
      this.myActorIndex = -1;

      // TODO: move to a separate module
      Game.quitToMainMenu();
    }
  }

  UpdateMovement(msg: messages.UpdateMovementMessage): void {
    const i = msg.idx;
    this.forms[i].movement = msg.data;
    if (!this.forms[i].numMovementChanges) {
      this.forms[i].numMovementChanges = 0;
    }
    this.forms[i].numMovementChanges++;
  }

  UpdateAnimation(msg: messages.UpdateAnimationMessage): void {
    const i = msg.idx;
    this.forms[i].animation = msg.data;
  }

  UpdateLook(msg: messages.UpdateLookMessage): void {
    const i = msg.idx;
    this.forms[i].look = msg.data;
  }

  UpdateEquipment(msg: messages.UpdateEquipmentMessage): void {
    const i = msg.idx;
    this.forms[i].equipment = msg.data;
  }

  handleConnectionAccepted(): void {
    this.forms = [];
    this.myActorIndex = -1;

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
    return { forms: this.forms, playerCharacterFormIdx: this.myActorIndex };
  }

  getMyActorIndex(): number {
    return this.myActorIndex;
  }

  send(msg: Record<string, unknown>, reliable: boolean): void {
    if (this.myActorIndex === -1) return;

    msg.idx = this.myActorIndex;
    networking.send(msg, reliable);
  }

  private forms = new Array<FormModelInfo>();
  private myActorIndex = -1;
}
