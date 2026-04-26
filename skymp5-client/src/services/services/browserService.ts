
// TODO: send event instead of direct dependency on FormView class
import { FormView } from "../../view/formView";
import { logError } from "../../logging";
import { MsgType } from "../../messages";
import { QueryKeyCodeBindings } from "../events/queryKeyCodeBindings";
import { CustomPacketMessage } from "../messages/customPacketMessage";

import { ClientListener, CombinedController, Sp } from "./clientListener";
import { BrowserMessageEvent, DxScanCode, Menu, MenuCloseEvent, MenuOpenEvent } from "skyrimPlatform";

const unfocusEventString = `window.dispatchEvent(new CustomEvent('skymp5-client:browserUnfocused', {}))`;
const focusEventString = `window.dispatchEvent(new CustomEvent('skymp5-client:browserFocused', {}))`;
const browserCustomPacketTypes: { [type: string]: true } = {
  "cef::chat:send": true,
};

export class BrowserService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    this.sp.browser.setVisible(false);

    this.controller.emitter.on("queryKeyCodeBindings", (e) => this.onQueryKeyCodeBindings(e));
    this.controller.once("update", () => this.onceUpdate());
    this.controller.on("browserMessage", (e) => this.onBrowserMessage(e));
    this.controller.on("menuOpen", (e) => this.onMenuOpen(e));
    this.controller.on("menuClose", (e) => this.onMenuClose(e));
  }

  // TODO: keycodes should be configurable
  private onQueryKeyCodeBindings(e: QueryKeyCodeBindings) {
    if (e.isDown([DxScanCode.F1])) {
      FormView.isDisplayingNicknames = !FormView.isDisplayingNicknames;
    }
    if (e.isDown([DxScanCode.F2])) {
      this.sp.browser.setVisible(!this.sp.browser.isVisible());
    }
    if (this.badMenusOpen.size === 0 && e.isDown([DxScanCode.F6])) {
      const newState = !this.sp.browser.isFocused();
      this.sp.browser.setFocused(newState);
      if (newState) {
        this.sp.browser.executeJavaScript(focusEventString);
      } else {
        this.sp.browser.executeJavaScript(unfocusEventString);
      }
    }
    if (this.badMenusOpen.size === 0 && e.isDown([DxScanCode.Enter])) {
      this.sp.browser.setFocused(true);
      this.sp.browser.executeJavaScript(focusEventString);
    }
    if (e.isDown([DxScanCode.Escape])) {
      if (this.sp.browser.isFocused()) {
        this.sp.browser.setFocused(false);
        this.sp.browser.executeJavaScript(unfocusEventString);
      }
    }
  }

  private onceUpdate() {
    this.sp.browser.setVisible(true);
  }

  private onBrowserMessage(e: BrowserMessageEvent) {
    const onFrontLoadedEventKey = "front-loaded";
    const arg = e.arguments[0];

    if (arg === onFrontLoadedEventKey) {
      this.controller.emitter.emit("browserWindowLoaded", {});
      return;
    }

    if (this.tryForwardBrowserCustomPacketArgs(e.arguments)) {
      return;
    }

    if (this.tryForwardBrowserCustomPacket(arg)) {
      return;
    }
  }

  private tryForwardBrowserCustomPacketArgs(args: unknown[]): boolean {
    const type = args[0];
    if (typeof type !== "string" || !browserCustomPacketTypes[type]) {
      return false;
    }

    return this.forwardBrowserCustomPacket(type, args[1]);
  }

  private tryForwardBrowserCustomPacket(arg: unknown): boolean {
    if (!this.isBrowserMessagePayload(arg)) {
      return false;
    }

    if (!browserCustomPacketTypes[arg.type]) {
      return false;
    }

    return this.forwardBrowserCustomPacket(arg.type, arg.data);
  }

  private forwardBrowserCustomPacket(type: string, data: unknown): boolean {
    try {
      const message: CustomPacketMessage = {
        t: MsgType.CustomPacket,
        contentJsonDump: JSON.stringify({
          type,
          data,
        }),
      };

      this.controller.emitter.emit("sendMessage", {
        message,
        reliability: "reliable",
      });
    } catch (err) {
      logError(this, "Failed to forward browser custom packet", err);
    }

    return true;
  }

  private isBrowserMessagePayload(arg: unknown): arg is { type: string, data: unknown } {
    if (typeof arg !== "object" || arg === null) {
      return false;
    }

    const maybePayload = arg as { type?: unknown };
    return typeof maybePayload.type === "string";
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

  private badMenusOpen = new Set<string>();

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
