import { AuthGameData, authGameDataStorageKey } from "../../features/authModel";
import { FunctionInfo } from "../../lib/functionInfo";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { BrowserMessageEvent } from "skyrimPlatform";
import { AuthNeededEvent } from "../events/authNeededEvent";
import { logTrace, logError } from "../../logging";
import { ConnectionMessage } from "../events/connectionMessage";
import { CreateActorMessage } from "../messages/createActorMessage";
import { CustomPacketMessage } from "../messages/customPacketMessage";
import { NetworkingService } from "./networkingService";
import { SettingsService } from "./settingsService";
import { MsgType } from "../../messages";
import { ConnectionDenied } from "../events/connectionDenied";

// for browser-side widget setters
declare const window: any;

const events = {
  updateRequired: 'updateRequired',
  joinDiscord: 'joinDiscord',
};

let browserState = {
  loginFailedReason: '',
};

export class AuthService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    this.controller.emitter.on("authNeeded", (e) => this.onAuthNeeded(e));
    this.controller.emitter.on("preConnectDenied", (e) => this.onPreConnectDenied(e));
    this.controller.emitter.on("createActorMessage", (e) => this.onCreateActorMessage(e));
    this.controller.emitter.on("connectionAccepted", () => this.handleConnectionAccepted());
    this.controller.emitter.on("connectionDenied", (e) => this.handleConnectionDenied(e));
    this.controller.emitter.on("customPacketMessage", (e) => this.onCustomPacketMessage(e));
    this.controller.on("browserMessage", (e) => this.onBrowserMessage(e));
  }

  private onAuthNeeded(e: AuthNeededEvent) {
    logTrace(this, `Received authNeeded event`);

    const settingsGameData = this.sp.settings["skymp5-client"]["gameData"] as any;
    const isOfflineMode = Number.isInteger(settingsGameData?.profileId);

    if (isOfflineMode) {
      logTrace(this, `Offline mode detected in settings, emitting auth event with authGameData.local`);
      this.controller.emitter.emit("authAttempt", { authGameData: { local: { profileId: settingsGameData.profileId } } });
    } else if (typeof settingsGameData?.session === 'string') {
      logTrace(this, `Launcher session detected in settings, skipping login dialog`);
      this.controller.emitter.emit("authAttempt", {
        authGameData: {
          remote: {
            session: settingsGameData.session,
            masterApiId: 0,
            discordUsername: null,
            discordDiscriminator: null,
            discordAvatar: null,
          }
        }
      });
    } else {
      logTrace(this, `No credentials detected in settings, showing launcher-required message`);
      this.sp.browser.setVisible(true);
      this.sp.browser.setFocused(true);
      this.sp.browser.executeJavaScript(
        new FunctionInfo(this.launcherRequiredWidgetSetter).getText({})
      );
    }
  }

  private onPreConnectDenied(e: { reason: 'sessionInvalid' | 'serverLocked' }) {
    logTrace(this, `preConnectDenied:`, e.reason);
    if (e.reason === 'sessionInvalid') {
      this.showLoginFailed('session expired — please relaunch the Frostfall Launcher');
    } else {
      this.showLoginFailed('the server is currently locked');
    }
  }

  private onCreateActorMessage(e: ConnectionMessage<CreateActorMessage>) {
    if (e.message.isMe) {
      logTrace(this, `Received createActorMessage for self, resetting widgets`);
      this.sp.browser.executeJavaScript('window.skyrimPlatform.widgets.set([]);');
    }
  }

  private onCustomPacketMessage(event: ConnectionMessage<CustomPacketMessage>): void {
    const msg = event.message;

    let msgContent: Record<string, unknown> = {};

    try {
      msgContent = JSON.parse(msg.contentJsonDump);
    } catch (e) {
      if (e instanceof SyntaxError) {
        logError(this, "onCustomPacketMessage failed to parse JSON", e.message, "json:", msg.contentJsonDump);
        return;
      } else {
        throw e;
      }
    }

    switch (msgContent["customPacketType"]) {
      case 'loginFailedSessionNotFound':
        logTrace(this, 'loginFailedSessionNotFound received');
        this.showLoginFailed('session expired — please relaunch the Frostfall Launcher');
        break;
      case 'loginFailedNotInTheDiscordServer':
        logTrace(this, 'loginFailedNotInTheDiscordServer received');
        this.showLoginFailed('please join our Discord server');
        break;
      case 'loginFailedBanned':
        logTrace(this, 'loginFailedBanned received');
        this.showLoginFailed('you are banned');
        break;
      case 'loginFailedIpMismatch':
        logTrace(this, 'loginFailedIpMismatch received');
        this.showLoginFailed('connection error — please try again');
        break;
      case 'loginFailedServerLocked':
        logTrace(this, 'loginFailedServerLocked received');
        this.showLoginFailed('the server is currently locked');
        break;
    }
  }

  private onBrowserMessage(e: BrowserMessageEvent) {
    if (e.arguments[0] === events.updateRequired) {
      this.sp.win32.loadUrl("https://skymp.net/UpdInstall");
    } else if (e.arguments[0] === events.joinDiscord) {
      this.sp.win32.loadUrl("https://discord.gg/9KhSZ6zjGT");
    }
  }

  private showLoginFailed(reason: string) {
    this.controller.lookupListener(NetworkingService).close();
    this.controller.lookupListener(SettingsService).clearTargetPeerCache();
    browserState.loginFailedReason = reason;
    this.sp.browser.setVisible(true);
    this.sp.browser.setFocused(true);
    this.sp.browser.executeJavaScript(
      new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState })
    );
  }

  private deniedWidgetSetter = () => {
    const widget = {
      type: "form",
      id: 2,
      caption: "update available",
      elements: [
        { type: "text", text: "an update is available", tags: [] },
        { type: "text", text: "download it at", tags: [] },
        { type: "text", text: "skymp.net", tags: [] },
        {
          type: "button",
          text: "open skymp.net",
          tags: ["ELEMENT_STYLE_MARGIN_EXTENDED"],
          click: () => window.skyrimPlatform.sendMessage(events.updateRequired),
          hint: "Go to the update download page",
        }
      ]
    };
    window.skyrimPlatform.widgets.set([widget]);
    window.skyrimPlatform.widgets = null;
  }

  private loginFailedWidgetSetter = () => {
    const splitParts = browserState.loginFailedReason.split('\n');

    const textElements = splitParts.map((part) => ({
      type: "text",
      text: part,
      tags: [],
    }));

    const widget = {
      type: "form",
      id: 2,
      caption: "login failed",
      elements: new Array<any>()
    };

    textElements.forEach((element) => widget.elements.push(element));

    if (browserState.loginFailedReason === 'please join our Discord server') {
      widget.elements.push({
        type: "button",
        text: "join",
        tags: ["ELEMENT_STYLE_MARGIN_EXTENDED"],
        click: () => window.skyrimPlatform.sendMessage(events.joinDiscord),
        hint: null
      });
    }

    window.skyrimPlatform.widgets.set([widget]);
  }

  private launcherRequiredWidgetSetter = () => {
    const widget = {
      type: "form",
      id: 1,
      caption: "Frostfall",
      elements: [
        { type: "text", text: "please use the Frostfall Launcher to log in", tags: [] }
      ]
    };
    window.skyrimPlatform.widgets.set([widget]);
  }

  private handleConnectionDenied(e: ConnectionDenied) {
    if (e.error.toLowerCase().includes("invalid password")) {
      this.controller.once("tick", () => {
        this.controller.lookupListener(NetworkingService).close();
      });
      this.sp.browser.executeJavaScript(new FunctionInfo(this.deniedWidgetSetter).getText({ events }));
      this.sp.browser.setVisible(true);
      this.sp.browser.setFocused(true);
      this.controller.once("update", () => {
        this.sp.Game.disablePlayerControls(true, true, true, true, true, true, true, true, 0);
      });
    }
  }

  private handleConnectionAccepted() {
    const authData = this.sp.storage[authGameDataStorageKey] as AuthGameData | undefined;

    if (authData?.local) {
      logTrace(this, `Logging in offline mode, profileId =`, authData.local.profileId);
      const message: CustomPacketMessage = {
        t: MsgType.CustomPacket,
        contentJsonDump: JSON.stringify({
          customPacketType: 'loginWithSkympIo',
          gameData: {
            profileId: authData.local.profileId,
          },
        }),
      };
      this.controller.emitter.emit("sendMessage", { message, reliability: "reliable" });
      return;
    }

    if (authData?.remote) {
      logTrace(this, 'Logging in as a master API user');
      const message: CustomPacketMessage = {
        t: MsgType.CustomPacket,
        contentJsonDump: JSON.stringify({
          customPacketType: 'loginWithSkympIo',
          gameData: {
            session: authData.remote.session,
          },
        }),
      };
      this.controller.emitter.emit("sendMessage", { message, reliability: "reliable" });
      return;
    }

    logError(this, 'No authentication method found in storage');
    this.showLoginFailed('please use the Frostfall Launcher to log in');
  }
}
