import {
  browser,
  on,
  once,
  Input,
  printConsole,
  settings,
  Ui,
  UiMenu,
  DxScanCodes
} from "skyrimPlatform";
import { DXScanCodes } from '../lib/dx-scan-codes';
import { EventEmitter } from '../lib/event-emitter';
import { badMenus } from '../lib/ui-menu';

const badMenus: UiMenu[] = [
	UiMenu.Barter,
	UiMenu.Book,
	UiMenu.Container,
	UiMenu.Crafting,
	UiMenu.Gift,
	UiMenu.Inventory,
	UiMenu.Journal,
	UiMenu.Lockpicking,
	UiMenu.Loading,
	UiMenu.Map,
	UiMenu.RaceSex,
	UiMenu.Stats,
	UiMenu.Tween,
	UiMenu.Console,
	UiMenu.Main,
];

const getMultiBindigString = (...codes: DXScanCodes[]) => codes.sort((a, b) => a - b).join('-');

export const main = (): void => {
  const emitter = new EventEmitter();

  let noBadMenuOpen = true;
  let lastBadMenuCheck = 0;

  let visible = false;
  const browserSetVisible = (state: boolean) => {
    visible = state;
    browser.setVisible(state);
  };
  browser.setVisible(false);
  once("update", () => {
    visible = true;
    browser.setVisible(true);
  });

  let focused = false;
  const browserSetFocused = (state: boolean) => {
    focused = state;
    browser.setFocused(state);
  };

  const inputChangeEvent = 'event:input-change';
  const keyState: { num: number } = { num: 0 };

  on('update', () => {
    const numKeys = Input.getNumKeysPressed();

    if (keyState.num !== numKeys) {
      keyState.num = numKeys;
      const keyCodes = Array(numKeys)
        .fill(null)
        .map((_, i) => Input.getNthKeyPressed(i));
      emitter.emit(inputChangeEvent, keyCodes);
    }

    if (Date.now() - lastBadMenuCheck > 200) {
      lastBadMenuCheck = Date.now();
      noBadMenuOpen = badMenus.findIndex((menu) => Ui.isMenuOpen(menu)) === -1;
    }

    browser.setVisible(visible && noBadMenuOpen)
  });

  const singleBindings: Record<number, () => void> = {
    [DXScanCodes.F2]: () => browserSetVisible(!visible),
    [DXScanCodes.F6]: () => browserSetFocused(!focused),
    [DXScanCodes.Escape]: () => (focused ? browserSetFocused(false) : undefined),
  };

  const multiBindings: Record<string, () => void> = {
    [getMultiBindigString(DXScanCodes.LeftShift, DXScanCodes.Tab)]: () => {
      // example use multi keys binding
    },
  };

  emitter.subscribe(inputChangeEvent, (data) => {
    if (!Array.isArray(data)) return;

    const keycodes: number[] = data;
    if (keycodes.length === 0) return;

    const multi: string = keycodes.join('-');

    if (multiBindings[multi]) {
      multiBindings[multi]();
      return;
    }

    const single: number = keycodes[0];
    if (singleBindings[single]) singleBindings[single]();
  });

  const cfg = {
    ip: settings["skymp5-client"]["server-ip"],
    port: settings["skymp5-client"]["server-port"],
  };

  printConsole({ cfg });

  const uiPort = cfg.port === 7777 ? 3000 : cfg.port as number + 1;

  const url = `http://${cfg.ip}:${uiPort}/ui/index.html`;
  printConsole(`loading url ${url}`);
  browser.loadUrl(url);
};
