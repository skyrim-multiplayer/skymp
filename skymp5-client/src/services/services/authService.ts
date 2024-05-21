import { AuthGameData, RemoteAuthGameData, authGameDataStorageKey } from "../../features/authModel";
import { FunctionInfo } from "../../lib/functionInfo";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { BrowserMessageEvent, Menu } from "skyrimPlatform";
import { AuthNeededEvent } from "../events/authNeededEvent";
import { BrowserWindowLoadedEvent } from "../events/browserWindowLoadedEvent";
import { TimersService } from "./timersService";
import { MasterApiAuthStatus } from "../messages_http/masterApiAuthStatus";
import { logTrace, logError } from "../../logging";
import { ConnectionMessage } from "../events/connectionMessage";
import { CreateActorMessage } from "../messages/createActorMessage";
import { CustomPacketMessage } from "../messages/customPacketMessage";
import { NetworkingService } from "./networkingService";
import { CustomPacketMessage2 } from "../messages/customPacketMessage2";
import { MsgType } from "../../messages";
import { ConnectionDenied } from "../events/connectionDenied";

// for browsersideWidgetSetter
declare const window: any;

// Constants used on both client and browser side (see browsersideWidgetSetter)
const events = {
  openDiscordOauth: 'openDiscordOauth',
  authAttempt: 'authAttemptEvent',
  openGithub: 'openGithub',
  openPatreon: 'openPatreon',
  clearAuthData: 'clearAuthData',
  updateRequired: 'updateRequired',
  backToLogin: 'backToLogin',
};

// Vaiables used on both client and browser side (see browsersideWidgetSetter)
let browserState = {
  comment: '',
  failCount: 9000,
  loginFailedReason: '',
};
let authData: RemoteAuthGameData | null = null;

export class AuthService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    this.controller.emitter.on("authNeeded", (e) => this.onAuthNeeded(e));
    this.controller.emitter.on("browserWindowLoaded", (e) => this.onBrowserWindowLoaded(e));
    this.controller.emitter.on("createActorMessage", (e) => this.onCreateActorMessage(e));
    this.controller.emitter.on("connectionAccepted", () => this.handleConnectionAccepted());
    this.controller.emitter.on("connectionDenied", (e) => this.handleConnectionDenied(e));
    this.controller.emitter.on("customPacketMessage2", (e) => this.onCustomPacketMessage2(e));
    this.controller.on("browserMessage", (e) => this.onBrowserMessage(e));
    this.controller.on("tick", () => this.onTick());
  }

  private onAuthNeeded(e: AuthNeededEvent) {
    logTrace(this, `Received authNeeded event`);

    const settingsGameData = this.sp.settings["skymp5-client"]["gameData"] as any;
    const isOfflineMode = Number.isInteger(settingsGameData?.profileId);
    if (isOfflineMode) {
      logTrace(this, `Offline mode detected in settings, emitting auth event with authGameData.local`);
      this.controller.emitter.emit("authAttempt", { authGameData: { local: { profileId: settingsGameData.profileId } } });
    } else {
      logTrace(this, `No offline mode detectted in settings, regular auth needed`);
      this.isListenBrowserMessage = true;

      this.trigger.authNeededFired = true;
      if (this.trigger.conditionMet) {
        this.onBrowserWindowLoadedAndOnlineAuthNeeded();
      }
    }
  }

  private onBrowserWindowLoaded(e: BrowserWindowLoadedEvent) {
    logTrace(this, `Received browserWindowLoaded event`);

    this.trigger.browserWindowLoadedFired = true;
    if (this.trigger.conditionMet) {
      this.onBrowserWindowLoadedAndOnlineAuthNeeded();
    }
  }

  private onCreateActorMessage(e: ConnectionMessage<CreateActorMessage>) {
    if (e.message.isMe) {
      if (this.authDialogOpen) {
        logTrace(this, `Received createActorMessage for self, resetting widgets`);
        this.sp.browser.executeJavaScript('window.skyrimPlatform.widgets.set([]);');
        this.authDialogOpen = false;
      }
      else {
        logTrace(this, `Received createActorMessage for self, but auth dialog was not open so not resetting widgets`);
      }
    }

    this.loggingStartMoment = 0;
  }

  private onCustomPacketMessage2(event: ConnectionMessage<CustomPacketMessage2>): void {
    const msg = event.message;

    switch (msg.content["customPacketType"]) {
      // case 'loginRequired':
      //   logTrace(this, 'loginRequired received');
      //   this.loginWithSkympIoCredentials();
      //   break;

      // const loginFailedNotLoggedViaDiscord = JSON.stringify({ customPacketType: "loginFailedNotLoggedViaDiscord" });
      // const loginFailedNotInTheDiscordServer = JSON.stringify({ customPacketType: "loginFailedNotInTheDiscordServer" });
      // const loginFailedBanned = JSON.stringify({ customPacketType: "loginFailedBanned" });
      // const loginFailedIpMismatch = JSON.stringify({ customPacketType: "loginFailedIpMismatch" });
      

      case 'loginFailedNotLoggedViaDiscord':
        logTrace(this, 'loginFailedNotLoggedViaDiscord received');
        browserState.comment = 'войдите через discord';
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData }));
        break;
      case 'loginFailedNotInTheDiscordServer':
        logTrace(this, 'loginFailedNotInTheDiscordServer received');
        browserState.comment = 'вступите в discord сервер';
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData }));
        break;
      case 'loginFailedBanned':
        logTrace(this, 'loginFailedBanned received');
        browserState.comment = 'вы забанены';
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData }));
        break;
      case 'loginFailedIpMismatch':
        logTrace(this, 'loginFailedIpMismatch received');
        browserState.comment = 'что это было?';
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData }));
        break;
    }
  }

  private onBrowserWindowLoadedAndOnlineAuthNeeded() {
    if (!this.isListenBrowserMessage) {
      logError(this, `isListenBrowserMessage was false for some reason, aborting auth`);
      return;
    }

    logTrace(this, `Showing widgets and starting loop`);

    authData = this.readAuthDataFromDisk();
    this.refreshWidgets();
    this.sp.browser.setVisible(true);
    this.sp.browser.setFocused(true);

    const timersService = this.controller.lookupListener(TimersService);

    logTrace(this, "Calling setTimeout for testing");
    try {
      timersService.setTimeout(() => {
        logTrace(this, "Test timeout fired");
      }, 1);
    }
    catch (e) {
      logError(this, "Failed to call setTimeout");
    }

    // Launch checkLoginState loop
    this.checkLoginState();
  }

  private onBrowserMessage(e: BrowserMessageEvent) {
    if (!this.isListenBrowserMessage) {
      logTrace(this, `onBrowserMessage: isListenBrowserMessage was false, ignoring message`, JSON.stringify(e.arguments));
      return;
    }

    logTrace(this, `onBrowserMessage:`, JSON.stringify(e.arguments));

    const eventKey = e.arguments[0];
    switch (eventKey) {
      case events.openDiscordOauth:
        this.sp.win32.loadUrl(`${this.getMasterUrl()}/api/users/login-discord?state=${this.discordAuthState}`);
        break;
      case events.authAttempt:
        if (authData === null) {
          browserState.comment = 'сначала войдите через discord';
          this.refreshWidgets();
          break;
        }
        this.writeAuthDataToDisk(authData);
        this.controller.emitter.emit("authAttempt", { authGameData: { remote: authData } });
        break;
      case events.clearAuthData:
        this.writeAuthDataToDisk(null);
        break;
      case events.openGithub:
        this.sp.win32.loadUrl(this.githubUrl);
        break;
      case events.openPatreon:
        this.sp.win32.loadUrl(this.patreonUrl);
        break;
      case events.updateRequired:
        this.sp.win32.loadUrl("https://skymp.net/");
        break;
      case events.backToLogin:
        this.sp.browser.executeJavaScript(new FunctionInfo(this.browsersideWidgetSetter).getText({ events, browserState, authData: authData }));
        break;
      default:
        logError(this, `Unknown event key`, eventKey);
        break;
    }
  }

  private createPlaySession(token: string, callback: (res: string, err: string) => void) {
    const client = new this.sp.HttpClient(this.getMasterUrl());
    let masterKey = this.sp.settings["skymp5-client"]["server-master-key"];
    if (!masterKey) {
      masterKey = this.sp.settings["skymp5-client"]["master-key"];
    }
    if (!masterKey) {
      masterKey = this.sp.settings["skymp5-client"]["server-ip"] + ":" + this.sp.settings["skymp5-client"]["server-port"];
    }

    const route = `/api/users/me/play/${masterKey}`;

    logTrace(this, `Creating play session ${route}`);

    client.post(route, {
      body: '{}',
      contentType: 'application/json',
      headers: {
        'authorization': token,
      },
      // @ts-ignore
    }, (res) => {
      if (res.status != 200) {
        callback('', 'status code ' + res.status);
      }
      else {
        // TODO: handle JSON.parse failure?
        callback(JSON.parse(res.body).session, '');
      }
    });
  }

  private checkLoginState() {
    if (!this.isListenBrowserMessage) {
      logTrace(this, `checkLoginState: isListenBrowserMessage was false, aborting check`);
      return;
    }

    const timersService = this.controller.lookupListener(TimersService);

    new this.sp.HttpClient(this.getMasterUrl())
      .get("/api/users/login-discord/status?state=" + this.discordAuthState, undefined,
        // @ts-ignore
        (response) => {
          switch (response.status) {
            case 200:
              const {
                token,
                masterApiId,
                discordUsername,
                discordDiscriminator,
                discordAvatar,
              } = JSON.parse(response.body) as MasterApiAuthStatus;
              browserState.failCount = 0;
              this.createPlaySession(token, (playSession, error) => {
                if (error) {
                  browserState.failCount = 0;
                  browserState.comment = (error);
                  timersService.setTimeout(() => this.checkLoginState(), 1.5 + Math.random() * 2);
                  this.refreshWidgets();
                  return;
                }
                authData = {
                  session: playSession,
                  masterApiId,
                  discordUsername,
                  discordDiscriminator,
                  discordAvatar,
                };
                browserState.comment = 'привязан успешно';
                this.refreshWidgets();
              });
              break;
            case 401: // Unauthorized
              browserState.failCount = 0;
              browserState.comment = (`Still waiting...`);
              timersService.setTimeout(() => this.checkLoginState(), 1.5 + Math.random() * 2);
              break;
            case 403: // Forbidden
            case 404: // Not found
              browserState.failCount = 9000;
              browserState.comment = (`Fail: ${response.body}`);
              break;
            default:
              ++browserState.failCount;
              browserState.comment = `Server returned ${response.status.toString() || "???"} "${response.body || response.error}"`;
              timersService.setTimeout(() => this.checkLoginState(), 1.5 + Math.random() * 2);
          }
        });
  };

  private refreshWidgets() {
    if (browserState.failCount) {
      logError(this, `Auth check fail:`, browserState.comment);
    }
    this.sp.browser.executeJavaScript(new FunctionInfo(this.browsersideWidgetSetter).getText({ events, browserState, authData: authData }));
    this.authDialogOpen = true;
  };

  private readAuthDataFromDisk(): RemoteAuthGameData | null {
    logTrace(this, `Reading`, this.pluginAuthDataName, `from disk`);

    try {
      const data = this.sp.getPluginSourceCode(this.pluginAuthDataName);

      if (!data) {
        logTrace(this, `Read empty`, this.pluginAuthDataName, `returning null`);
        return null;
      }

      return JSON.parse(data.slice(2)) || null;
    } catch (e) {
      logError(this, `Error reading`, this.pluginAuthDataName, `from disk:`, e, `, falling back to null`);
      return null;
    }
  }

  private writeAuthDataToDisk(data: RemoteAuthGameData | null) {
    const content = "//" + (data ? JSON.stringify(data) : "null");

    logTrace(this, `Writing`, this.pluginAuthDataName, `to disk:`, content);

    try {
      this.sp.writePlugin(
        this.pluginAuthDataName,
        content
      );
    }
    catch (e) {
      logError(this, `Error writing`, this.pluginAuthDataName, `to disk:`, e, `, will not remember user`);
    }
  };

  private getMasterUrl() {
    return this.normalizeUrl((this.sp.settings["skymp5-client"]["master"] as string) || "https://sweetpie.nic11.xyz");
  }

  private normalizeUrl(url: string) {
    if (url.endsWith('/')) {
      return url.slice(0, url.length - 1);
    }
    return url;
  };

  private deniedWidgetSetter = () => {
    const widget = {
      type: "form",
      id: 2,
      caption: "новинка",
      elements: [
        {
          type: "text",
          text: "ура! вышло обновление",
          tags: []
        },
        {
          type: "text",
          text: "спешите скачать на skymp.net",
          tags: []
        },
        {
          type: "button",
          text: "открыть skymp.net",
          tags: ["ELEMENT_STYLE_MARGIN_EXTENDED"],
          click: () => window.skyrimPlatform.sendMessage(events.updateRequired),
          hint: "Перейти на страницу скачивания обновления",
        }
      ]
    }
    window.skyrimPlatform.widgets.set([widget]);

    // Make sure gamemode will not be able to update widgets anymore
    window.skyrimPlatform.widgets = null;
  }

  private loginFailedWidgetSetter = () => {
    const widget = {
      type: "form",
      id: 2,
      caption: "Авторизация",
      elements: [
        {
          type: "text",
          text: browserState.loginFailedReason,
          tags: []
        },
        {
          type: "button",
          text: "назад",
          tags: ["ELEMENT_STYLE_MARGIN_EXTENDED"],
          click: () => window.skyrimPlatform.sendMessage(events.backToLogin),
          hint: undefined
        }
      ]
    }
    window.skyrimPlatform.widgets.set([widget]);
  }

  private browsersideWidgetSetter = () => {
    console.log(new Date());
    const loginWidget = {
      type: "form",
      id: 1,
      caption: "Авторизация",
      elements: [
        // {
        //   type: "button",
        //   tags: ["BUTTON_STYLE_GITHUB"],
        //   hint: "get a colored nickname and mention in news",
        //   click: () => window.skyrimPlatform.sendMessage(events.openGithub),
        // },
        // {
        //   type: "button",
        //   tags: ["BUTTON_STYLE_PATREON", "ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT"],
        //   hint: "get a colored nickname and other bonuses for patrons",
        //   click: () => window.skyrimPlatform.sendMessage(events.openPatreon),
        // },
        // {
        //   type: "icon",
        //   text: "username",
        //   tags: ["ICON_STYLE_SKYMP"],
        // },
        // {
        //   type: "icon",
        //   text: "",
        //   tags: ["ICON_STYLE_DISCORD"],
        // },
        {
          type: "text",
          text: (
            authData ? (
              authData.discordUsername
                ? `${authData.discordUsername}`
                : `id: ${authData.masterApiId}`
            ) : "не авторизирован"
          ),
          tags: [/*"ELEMENT_SAME_LINE", "ELEMENT_STYLE_MARGIN_EXTENDED"*/],
        },
        // {
        //   type: "icon",
        //   text: "discord",
        //   tags: ["ICON_STYLE_DISCORD"],
        // },
        {
          type: "button",
          text: authData ? "сменить аккаунт" : "войти через discord",
          tags: [/*"ELEMENT_SAME_LINE"*/],
          click: () => window.skyrimPlatform.sendMessage(events.openDiscordOauth),
          hint: "Вы можете войти или поменять аккаунт",
        },
        {
          type: "button",
          text: "Играть",
          tags: ["BUTTON_STYLE_FRAME", "ELEMENT_STYLE_MARGIN_EXTENDED"],
          click: () => window.skyrimPlatform.sendMessage(events.authAttempt),
          hint: "Подключиться к игровому серверу",
        },
        {
          type: "text",
          text: browserState.comment,
          tags: [],
        },
      ]
    };
    console.log(loginWidget);
    window.skyrimPlatform.widgets.set([loginWidget]);
  };

  private handleConnectionAccepted() {
    this.loginWithSkympIoCredentials();
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

  private loginWithSkympIoCredentials() {
    this.loggingStartMoment = Date.now();

    const authData = this.sp.storage[authGameDataStorageKey] as AuthGameData | undefined;
    if (authData?.local) {
      logTrace(this,
        `Logging in offline mode, profileId =`, authData.local.profileId
      );
      const message: CustomPacketMessage = {
        t: MsgType.CustomPacket,
        content: {
          customPacketType: 'loginWithSkympIo',
          gameData: {
            profileId: authData.local.profileId,
          },
        },
      };
      this.controller.emitter.emit("sendMessage", {
        message: message,
        reliability: "reliable"
      });
      return;
    }

    if (authData?.remote) {
      logTrace(this, 'Logging in as a master API user');
      const message: CustomPacketMessage = {
        t: MsgType.CustomPacket,
        content: {
          customPacketType: 'loginWithSkympIo',
          gameData: {
            session: authData.remote.session,
          },
        },
      };
      this.controller.emitter.emit("sendMessage", {
        message: message,
        reliability: "reliable"
      });
      return;
    }

    logError(this, 'Not found authentication method');
  };

  private onTick() {
    // TODO: Should be no hardcoded/magic-number limit
    // TODO: Busy waiting is bad. Should be replaced with some kind of event
    const maxLoggingDelay = 15000;
    if (this.loggingStartMoment && Date.now() - this.loggingStartMoment > maxLoggingDelay) {
      logError(this, 'Logging in failed. Reconnecting.');
      this.showConnectionError();
      this.controller.lookupListener(NetworkingService).reconnect();
      this.loggingStartMoment = 0;
    }
  }

  private showConnectionError() {
    // TODO: unhardcode it or render via browser
    this.sp.printConsole("Server connection failed. This may be caused by one of the following:");
    this.sp.printConsole("1. You are not present on the SkyMP Discord server");
    this.sp.printConsole("2. You have been banned by server admins");
    this.sp.printConsole("3. There is some technical issue. Try linking your Discord account again");
    this.sp.printConsole("If you feel that something is wrong, please contact us on Discord.");
  };

  private isListenBrowserMessage = false;
  private trigger = {
    authNeededFired: false,
    browserWindowLoadedFired: false,

    get conditionMet() {
      return this.authNeededFired && this.browserWindowLoadedFired
    }
  };
  private discordAuthState = "" + Math.random();
  private authDialogOpen = false;

  private loggingStartMoment = 0;

  private readonly githubUrl = "https://github.com/skyrim-multiplayer/skymp";
  private readonly patreonUrl = "https://www.patreon.com/skymp";
  private readonly pluginAuthDataName = `auth-data-no-load`;
}
