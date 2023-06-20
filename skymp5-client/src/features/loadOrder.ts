import { Game, Utility, HttpClient, printConsole, createText } from "skyrimPlatform";
import * as sp from "skyrimPlatform";
import { getServerIp, getServerUiPort } from "../skympClient";
import { getScreenResolution } from "../view/formView";

const STATE_KEY = 'loadOrderCheckState';

interface State {
  statusTextId?: number;
};

const getState = (): State => {
  if (typeof sp.storage[STATE_KEY] !== 'object') {
    return {};
  }
  return sp.storage[STATE_KEY] as State;
};

const setState = (replacement: State) => {
  const oldState = sp.storage[STATE_KEY] = getState();
  for (const [k, v] of Object.entries(replacement)) {
    (oldState as Record<string, any>)[k] = v;
  }
};

const resetText = () => {
  let { statusTextId } = getState();
  if (statusTextId) {
    sp.destroyText(statusTextId);
    statusTextId = undefined;
    setState({ statusTextId });
  }
};

const updateText = (text: string, color: [number, number, number, number], clearDelay?: number) => {
  const { width, height } = getScreenResolution();
  resetText();
  const statusTextId = createText(width / 2, height / 2, text, color);
  setState({ statusTextId });
  if (clearDelay) {
    Utility.wait(clearDelay).then(resetText);
  }
}

interface Mod {
  filename: string;
  size: number;
  crc32: number;
};

interface ServerManifest {
  versionMajor: number;
  mods: Mod[];
  loadOrder: string[];
};

const getServerMods = (retriesLeft: number): Promise<Mod[]> => {
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
          .then(() => getServerMods(retriesLeft - 1));
      }
      return [];
    });
};

const enumerateClientMods = (getCount: (() => number), getAt: ((idx: number) => string)) => {
  const result = [];
  for (let i = 0; i < getCount(); ++i) {
    const filename = getAt(i);
    const { crc32, size } = sp.getFileInfo(filename);
    result.push({ filename, crc32, size });
  }
  return result;
}

const getClientMods = () => {
  return enumerateClientMods(Game.getModCount, Game.getModName);
};

const printModOrder = (header: string, order: Mod[]) => {
  printConsole(header);
  for (const [i, mod] of Object.entries(order)) {
    printConsole(`#${i} ${JSON.stringify(mod)}`);
  }
};

export const verifyLoadOrder = () => {
  resetText();
  const clientMods = getClientMods();
  printModOrder('Client load order:', clientMods);
  return getServerMods(5)
    .then((serverMods) => {
      printModOrder('Server load order:', serverMods);
      if (clientMods.length < serverMods.length) {
        throw new Error(`Missing some server mods. Server has ${serverMods.length}, we have ${clientMods.length}`);
      }
      if (clientMods.length > serverMods.length) {
        updateText(
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
      if (sp.settings['skymp5-client']['ignoreLoadOrderMismatch']) {
        updateText(
          'LOAD ORDER ERROR!\nHowever, ignoring it because of ignoreLoadOrderMismatch being set.' +
          '\nExpect EVERYTHING BREAK, unless you know what you are doing.\nCheck console for details.' +
          '\nThis message will disappear after 30 seconds.',
          [255, 0, 0, 1], 30,
        );
        return;
      }
      updateText(
        'LOAD ORDER ERROR!\nCheck console for details.',
        [255, 0, 0, 1]
      );
      sp.browser.loadUrl('about:blank');
    });
};
