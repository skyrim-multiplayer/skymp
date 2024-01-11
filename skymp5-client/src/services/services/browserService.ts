
// TODO: send event instead of direct dependency on FormView class
import { FormView } from "../../view/formView";

import { ClientListener, CombinedController, Sp } from "./clientListener";
import { BrowserMessageEvent, DxScanCode, Menu, MenuCloseEvent, MenuOpenEvent } from "skyrimPlatform";

export class BrowserService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    this.sp.browser.setVisible(false);

    this.controller.once("update", () => this.onceUpdate());
    this.controller.on("update", () => this.onUpdate());
    this.controller.on("browserMessage", (e) => this.onBrowserMessage(e));
    this.controller.on("menuOpen", (e) => this.onMenuOpen(e));
    this.controller.on("menuClose", (e) => this.onMenuClose(e));
  }

  private onceUpdate() {
    this.sp.browser.setVisible(true);
  }

  private onUpdate() {
    const numKeys = this.sp.Input.getNumKeysPressed();

    if (this.lastNumKeys === numKeys) return;

    this.lastNumKeys = numKeys;

    this.binding.forEach((fn, keyCodes) => {
      if (keyCodes.every((key) => this.sp.Input.isKeyPressed(key))) fn();
    });
  }

  private onBrowserMessage(e: BrowserMessageEvent) {
    const onFrontLoadedEventKey = "front-loaded";

    if (e.arguments[0] === onFrontLoadedEventKey) {
      this.controller.emitter.emit("browserWindowLoaded", {});
    }
  }

  private onMenuOpen(e: MenuOpenEvent) {
    if (this.isBadMenu(e.name)) {
      this.sp.browser.setVisible(false);
      this.badMenusOpen.add(e.name);
    } else if (e.name === Menu.HUD) {
      this.sp.browser.setVisible(true);
    }
  }

  private onMenuClose(e: MenuCloseEvent) {
    if (this.badMenusOpen.delete(e.name)) {
      if (this.badMenusOpen.size === 0) {
        this.sp.browser.setVisible(true);
      }
    }

    if (e.name === Menu.HUD) {
      this.sp.browser.setVisible(false);
    }
  }

  private isBadMenu(menu: string) {
    return this.badMenus.includes(menu as Menu);
  }

  private lastNumKeys = 0;

  private badMenusOpen = new Set<string>();

  // TODO: keycodes should be configurable
  private binding = new Map<DxScanCode[], () => void>([
    [[DxScanCode.F1], () => FormView.isDisplayingNicknames = !FormView.isDisplayingNicknames],
    [[DxScanCode.F2], () => this.sp.browser.setVisible(!this.sp.browser.isVisible())],
    [[DxScanCode.F6], () => this.sp.browser.setFocused(!this.sp.browser.isFocused())],
    [[DxScanCode.Enter], () => {
      if (this.badMenusOpen.size === 0) {
        this.sp.browser.setFocused(true);
      }
    }],
    [
      [DxScanCode.Escape],
      () => this.sp.browser.isFocused() && this.sp.browser.setFocused(false),
    ],
  ]);

  private readonly badMenus: Menu[] = [
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
}
