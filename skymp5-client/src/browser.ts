import {
  browser,
  on,
  once,
  Input,
  printConsole,
  settings,
  Ui,
  Menu,
  DxScanCode
} from "skyrimPlatform";

export const main = (): void => {
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
    Menu.Main
  ];

  let browserVisibleState = false;
  browser.setVisible(false);
  const setBrowserVisible = (state: boolean) => {
    browserVisibleState = state;
    browser.setVisible(state);
  };
  once('update', () => {
    browserVisibleState = true;
    browser.setVisible(true);
  });

  let browserFocusedState = false;
  const setBrowserFocused = (state: boolean) => {
    browserFocusedState = state;
    browser.setFocused(state);
  };

  let badMenuOpen = true;

  let lastBadMenuCheck = 0;
  on('update', () => {
    if (Date.now() - lastBadMenuCheck > 200) {
      lastBadMenuCheck = Date.now();

      badMenuOpen = false;
      for (let i = 0; i < badMenus.length; ++i) {
        if (Ui.isMenuOpen(badMenus[i])) {
          badMenuOpen = true;
          break;
        }
      }
      browser.setVisible(browserVisibleState && !badMenuOpen);
    }
  });

  const binding = new Map<DxScanCode[], () => void>();
  binding.set([DxScanCode.F2], () => setBrowserVisible(!browserVisibleState));
  binding.set([DxScanCode.F6], () => setBrowserFocused(!browserFocusedState));
  binding.set([DxScanCode.Escape], () => (browserFocusedState ? setBrowserFocused(false) : undefined));

  let lastNumKeys = 0;
  on('update', () => {
    const numKeys = Input.getNumKeysPressed();

    if (lastNumKeys === numKeys) return;

    lastNumKeys = numKeys;

    binding.forEach((fn, keyCodes) => {
      if (keyCodes.every(key => Input.isKeyPressed(key))) fn();
    });
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
