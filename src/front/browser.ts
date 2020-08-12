import {
  browser,
  on,
  once,
  Input,
  printConsole,
  settings,
  Ui,
} from "skyrimPlatform";

export const main = (): void => {
  const F2 = 0x3c;
  const F6 = 0x40;
  const Escape = 0x01;

  const badMenus = [
    "BarterMenu",
    "Book Menu",
    "ContainerMenu",
    "Crafting Menu",
    "GiftMenu",
    "InventoryMenu",
    "Journal Menu",
    "Lockpicking Menu",
    "Loading Menu",
    "MapMenu",
    "RaceSex Menu",
    "StatsMenu",
    "TweenMenu",
  ];

  browser.setVisible(false);
  let visible = false;
  let noBadMenuOpen = true;
  let lastBadMenuCheck = 0;

  once("update", () => {
    visible = true;
    browser.setVisible(true);
  });

  {
    let pressedWas = false;

    on("update", () => {
      const pressed = Input.isKeyPressed(F2);
      if (pressedWas !== pressed) {
        pressedWas = pressed;
        if (pressed) {
          visible = !visible;
        }
      }

      if (Date.now() - lastBadMenuCheck > 200) {
        lastBadMenuCheck = Date.now();
        noBadMenuOpen =
          badMenus.findIndex((menu) => Ui.isMenuOpen(menu)) === -1;
      }

      browser.setVisible(visible && noBadMenuOpen);
    });
  }

  {
    let focused = false;
    let pressedWas = false;

    on("update", () => {
      const pressed =
        Input.isKeyPressed(F6) || (focused && Input.isKeyPressed(Escape));
      if (pressedWas !== pressed) {
        pressedWas = pressed;
        if (pressed) {
          focused = !focused;
          browser.setFocused(focused);
        }
      }
    });
  }

  const url = `http://${settings["skymp5-client"]["server-ip"]}:3000/chat.html`;
  printConsole(`loading url ${url}`);
  browser.loadUrl(url);
};
