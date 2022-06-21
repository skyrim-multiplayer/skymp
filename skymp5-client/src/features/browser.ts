import {
  browser,
  on,
  once,
  Input,
  printConsole,
  settings,
  Menu,
  DxScanCode,
  writePlugin,
  getPluginSourceCode,
  MenuOpenEvent,
  MenuCloseEvent,
} from "skyrimPlatform";
import { FormView } from "../view/formView";
import { RemoteAuthGameData } from "./authModel";

const pluginAuthDataName = `auth-data-no-load`;
const onFrontLoadedEventKey = "front-loaded";

export type onWindowLoadedEventCallback = () => void;
const onWindowLoadListeners = new Array<onWindowLoadedEventCallback>();

type BindingKey = DxScanCode[];
type BindingValue = () => void;

const badMenus: Menu[] = [
  Menu.Barter,
  Menu.Book,
  Menu.Container,
  Menu.Crafting,
  Menu.Gift,
  Menu.Inventory,
  Menu.Journal,
  Menu.Lockpicking,
  Menu.Loading,
  Menu.Map,
  Menu.RaceSex,
  Menu.Stats,
  Menu.Tween,
  Menu.Console,
  Menu.Main,
];

const IsBadMenu = (menu: string) => badMenus.includes(menu as Menu);

on("browserMessage", (e) => {
  if (e.arguments[0] === onFrontLoadedEventKey) {
    onWindowLoadListeners.forEach(l => l());
  }
});

export const addOnWindowLoadListener = (listener: onWindowLoadedEventCallback): void => {
  onWindowLoadListeners.push(listener);
}

export const main = (): void => {
  browser.setVisible(false);
  once("update", () => browser.setVisible(true));

  const openedMenus: string[] = [];

  const badMenuOpen = () => !!openedMenus.length;

  on("menuOpen", (e: MenuOpenEvent) => {
    if (e.name === Menu.Cursor) {
      isCursorMenuOpened = true;
    }
    if (IsBadMenu(e.name)) {
      browser.setVisible(false);
      openedMenus.push(e.name);
    } else if (e.name === Menu.HUD) {
      browser.setVisible(true);
    }
  });

  on("menuClose", (e: MenuCloseEvent) => {
    if (e.name === Menu.Cursor) {
      isCursorMenuOpened = false;
    }
    const i = openedMenus.indexOf(e.name);
    if (i !== -1) {
      openedMenus.splice(i, 1);

      if (openedMenus.length === 0) browser.setVisible(true);
    }

    if (e.name === Menu.HUD) browser.setVisible(false);
  });

  const binding = new Map<BindingKey, BindingValue>([
    [[DxScanCode.F1], () => FormView.isDisplayingNicknames = !FormView.isDisplayingNicknames],
    [[DxScanCode.F2], () => browser.setVisible(!browser.isVisible())],
    [[DxScanCode.F6], () => browser.setFocused(!browser.isFocused())],
    [[DxScanCode.Enter], () => browser.setFocused(true)],
    [
      [DxScanCode.Escape],
      () => browser.isFocused() && browser.setFocused(false),
    ],
  ]);

  let lastNumKeys = 0;
  on("update", () => {
    const numKeys = Input.getNumKeysPressed();

    if (lastNumKeys === numKeys) return;

    lastNumKeys = numKeys;

    binding.forEach((fn, keyCodes) => {
      if (keyCodes.every((key) => Input.isKeyPressed(key))) fn();
    });
  });

  const cfg = {
    ip: settings["skymp5-client"]["server-ip"],
    port: settings["skymp5-client"]["server-port"],
  };

  printConsole({ cfg });

  const uiPort = cfg.port === 7777 ? 3000 : (cfg.port as number) + 1;

  const url = `http://${cfg.ip}:${uiPort}/ui/index.html`;
  printConsole(`loading url ${url}`);
  browser.loadUrl(url);
};

export const getAuthData = (): RemoteAuthGameData | null => {
  try {
    const data = getPluginSourceCode(pluginAuthDataName);
    if (data) {
      return JSON.parse(data.slice(2)) || null;
    }
  } catch (e) {
    printConsole(e);
    return null;
  }
  return null;
};

export const setAuthData = (data: RemoteAuthGameData | null): void => {
  printConsole(data);
  writePlugin(
    pluginAuthDataName,
    "//" + (data ? JSON.stringify(data) : "null")
  );
};

var isCursorMenuOpened = false;
export const keepCursorMenuOpenedWhenBrowserFocused = (): void => {
  once("update", () => {
    if (browser.isFocused() && !isCursorMenuOpened) {
      printConsole(`browser ${browser.isFocused()}, isCursorMenuOpened ${isCursorMenuOpened}`);
      browser.setFocused(false);
      once("update", () => {
        browser.setFocused(true);
        once("update", () => keepCursorMenuOpenedWhenBrowserFocused());
      });
    } else {
      keepCursorMenuOpenedWhenBrowserFocused();
    }
  });
}
