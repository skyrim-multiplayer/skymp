import { Game, Utility, HttpClient, printConsole, createText } from "skyrimPlatform";
import { getServerIp, getServerUiPort } from "./skympClient";
import { getScreenResolution } from "../../view/formView";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { Mod, ServerManifest } from "../messages_http/serverManifest";
import { logTrace } from "../../logging";

const STATE_KEY = 'loadOrderCheckState';

interface State {
  statusTextId?: number;
};

export class LoadOrderVerificationService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    this.controller.once("update", () => this.onceUpdate());
  }

  private onceUpdate() {
    this.verifyLoadOrder();
  }

  private verifyLoadOrder() {
    this.resetText();
    const clientMods = this.getClientMods();
    this.printModOrder('Client load order:', clientMods);
    return this.getServerMods(5)
      .then((serverMods) => {
        this.printModOrder('Server load order:', serverMods);
        if (clientMods.length < serverMods.length) {
          throw new Error(`Missing some server mods. Server has ${serverMods.length}, we have ${clientMods.length}`);
        }
        if (clientMods.length > serverMods.length) {
          this.updateText(
            'LOAD ORDER WARNING: you have more mods than server!\n(or could not receive server mod list)\nCheck console for details.',
            [255, 255, 0, 1], 5,
          );
        }
        let fail = [];
        for (let i = 0; i < serverMods.length; ++i) {
          // Need case-insensitive check for 1.6+
          if (
            clientMods[i].filename.toLowerCase() !== serverMods[i].filename.toLowerCase() ||
            clientMods[i].size !== serverMods[i].size ||
            clientMods[i].crc32 !== serverMods[i].crc32
          ) {
            fail.push(i);
            printConsole(`${i}-th mod (numbered from 0) does not match.`);
            printConsole(`Server has ${JSON.stringify(serverMods[i])}`);
            printConsole(`We have ${JSON.stringify(clientMods[i])}`);
          }
        }
        if (fail.length !== 0) {
          throw new Error('Load order check failed! Indices: ' + JSON.stringify(fail));
        }
      })
      .catch((err) => {
        printConsole(err);
        if (this.sp.settings['skymp5-client']['ignoreLoadOrderMismatch']) {
          this.updateText(
            'LOAD ORDER ERROR!\nHowever, ignoring it because of ignoreLoadOrderMismatch being set.' +
            '\nExpect EVERYTHING BREAK, unless you know what you are doing.\nCheck console for details.' +
            '\nThis message will disappear after 30 seconds.',
            [255, 0, 0, 1], 30,
          );
          return;
        }
        this.updateText(
          'LOAD ORDER ERROR!\nCheck console for details.',
          [255, 0, 0, 1]
        );
      });
  };

  private getState(): State {
    if (typeof this.sp.storage[STATE_KEY] !== 'object') {
      return {};
    }
    return this.sp.storage[STATE_KEY] as State;
  };

  private setState(replacement: State) {
    const oldState = this.sp.storage[STATE_KEY] = this.getState();
    for (const [k, v] of Object.entries(replacement)) {
      (oldState as Record<string, any>)[k] = v;
    }
  };

  private resetText() {
    let { statusTextId } = this.getState();
    if (statusTextId) {
      this.sp.destroyText(statusTextId);
      statusTextId = undefined;
      this.setState({ statusTextId });
    }
  };

  private updateText(text: string, color: [number, number, number, number], clearDelay?: number) {
    const { width, height } = getScreenResolution();
    this.resetText();
    const statusTextId = createText(width / 2, height / 2, text, color);
    this.setState({ statusTextId });
    if (clearDelay) {
      Utility.wait(clearDelay).then(() => this.resetText());
    }
  }

  private getServerMods(retriesLeft: number): Promise<Mod[]> {
    const targetIp = getServerIp();
    const uiPort = getServerUiPort();
    printConsole(`http://${targetIp}:${uiPort}`);
    return new HttpClient(`http://${targetIp}:${uiPort}`)
      .get('/manifest.json')
      .then((res) => {
        if (res.status != 200) {
          throw new Error(`Status code ${res.status}, error ${res.error}`);
        }
        const manifest = JSON.parse(res.body) as ServerManifest;
        if (manifest.versionMajor !== 1) {
          throw new Error(`Server manifest version is ${manifest.versionMajor}, we expect 1`);
        }
        return manifest.mods;
      })
      .catch((err) => {
        printConsole("Can't get server mods", err);
        if (retriesLeft > 0) {
          printConsole(`${retriesLeft} retries left...`);
          return Utility.wait(0.1 + Math.random())
            .then(() => this.getServerMods(retriesLeft - 1));
        }
        return [];
      });
  };

  private enumerateClientMods(getCount: (() => number), getAt: ((idx: number) => string)) {
    const result = [];
    for (let i = 0; i < getCount(); ++i) {
      const filename = getAt(i);
      const { crc32, size } = this.getFileInfoSafe(filename);
      result.push({ filename, crc32, size });
    }
    return result;
  }

  private getClientMods() {
    return this.enumerateClientMods(Game.getModCount, Game.getModName);
  };

  private printModOrder(header: string, order: Mod[]) {
    printConsole(header);
    for (const [i, mod] of Object.entries(order)) {
      printConsole(`#${i} ${JSON.stringify(mod)}`);
    }
  };

  private getFileInfoSafe(filename: string) {
    try {
      return this.sp.getFileInfo(filename);
    }
    catch (e) {
      // InvalidArgumentException.h, Skyrim Platform
      const searchString = 'is not a valid argument';

      const message = (e as Record<string, unknown>).message;

      if (typeof message === "string" && message.includes('is not a valid argument')) {
        logTrace(this, `Failed to get file info for`, filename);
        return { crc32: 0, size: 0 };
      }
      else {
        throw e;
      }
    }
  }
}
