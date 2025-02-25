import * as crypto from "crypto";
import * as fs from "fs";
import { AuthGameData, RemoteAuthGameData, authGameDataStorageKey } from "../../features/authModel";
import { FunctionInfo } from "../../lib/functionInfo";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { BrowserMessageEvent, Menu, browser } from "skyrimPlatform";
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
import { SettingsService } from "./settingsService";
import { RPCClientService } from "./rpcClientService";
import { RPCResultGetServerPassword } from "../messages_http/rpcResults/rpcResultGetServerPassword";

// for browsersideWidgetSetter
declare const window: any;

// Constants used on both client and browser side (see browsersideWidgetSetter)
const events = {
  'openDiscordOauth': 'openDiscordOauth',
  'authAttempt': 'authAttemptEvent',
  'openGithub': 'openGithub',
  'openPatreon': 'openPatreon',
  'clearAuthData': 'clearAuthData',
  'updateRequired': 'updateRequired',
  'updateSkip': 'updateSkip',
  'backToLogin': 'backToLogin',
  'joinDiscord': 'joinDiscord',
};

// Vaiables used on both client and browser side (see browsersideWidgetSetter)
let browserState = {
  'comment': '',
  'failCount': 9000,
  'loginFailedReason': '',
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
    this.controller.once("update", () => this.onceUpdate());
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
    this.authAttemptProgressIndicator = false;
  }

  private onCustomPacketMessage2(event: ConnectionMessage<CustomPacketMessage2>): void {
    const msg = event.message;

    switch (msg.content["customPacketType"]) {
      // case 'loginRequired':
      //   logTrace(this, 'loginRequired received');
      //   this.loginWithSkympIoCredentials();
      //   break;
      case 'loginFailedNotLoggedViaDiscord':
        this.authAttemptProgressIndicator = false;
        this.controller.lookupListener(NetworkingService).close();
        logTrace(this, 'loginFailedNotLoggedViaDiscord received');
        browserState.loginFailedReason = 'войдите через discord';
        browserState.comment = '';
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData }));
        break;
      case 'loginFailedNotInTheDiscordServer':
        this.authAttemptProgressIndicator = false;
        this.controller.lookupListener(NetworkingService).close();
        logTrace(this, 'loginFailedNotInTheDiscordServer received');
        browserState.loginFailedReason = 'вступите в discord сервер';
        browserState.comment = '';
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData }));
        break;
      case 'loginFailedBanned':
        this.authAttemptProgressIndicator = false;
        this.controller.lookupListener(NetworkingService).close();
        logTrace(this, 'loginFailedBanned received');
        browserState.loginFailedReason = 'вы забанены';
        browserState.comment = '';
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData }));
        break;
      case 'loginFailedIpMismatch':
        this.authAttemptProgressIndicator = false;
        this.controller.lookupListener(NetworkingService).close();
        logTrace(this, 'loginFailedIpMismatch received');
        browserState.loginFailedReason = 'что это было?';
        browserState.comment = '';
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

    const settingsService = this.controller.lookupListener(SettingsService);

    logTrace(this, `onBrowserMessage:`, JSON.stringify(e.arguments));

    const eventKey = e.arguments[0];
    switch (eventKey) {
      case events.openDiscordOauth:
        browserState.comment = 'открываем браузер...';
        this.refreshWidgets();
        this.sp.win32.loadUrl(`${settingsService.getMasterUrl()}/api/users/login-discord?state=${this.discordAuthState}`);
        break;
      case events.authAttempt:
        if (authData === null) {
          browserState.comment = 'сначала войдите';
          this.refreshWidgets();
          break;
        }
        this.writeAuthDataToDisk(authData);
        this.controller.emitter.emit("authAttempt", { authGameData: { remote: authData } });

        this.authAttemptProgressIndicator = true;

        break;
      case events.clearAuthData:
        // Doesn't seem to be used
        this.writeAuthDataToDisk(null);
        break;
      case events.openGithub:
        this.sp.win32.loadUrl(this.githubUrl);
        break;
      case events.openPatreon:
        this.sp.win32.loadUrl(this.patreonUrl);
        break;
      case events.updateRequired:
        this.sp.win32.loadUrl("https://skymp.net/UpdInstall");
        break;
      case events.updateSkip:
        browserState.comment = 'исправляем...';
        this.sp.browser.executeJavaScript(new FunctionInfo(this.browsersideWidgetSetter).getText({ events, browserState, authData: authData }));

        const rpcClientService = this.controller.lookupListener(RPCClientService);

        rpcClientService.callRpc<RPCResultGetServerPassword>("RPCGetServerPassword", {}, (response) => {
          if (!response) {
            logError(this, `Failed to get server password`);
            return;
          }

          // try backup password
          try {
            if (fs.existsSync("Data/Platform/Distribution/password")) {
              fs.copyFileSync("Data/Platform/Distribution/password", "Data/Platform/Distribution/password_backup");
            }
          }
          catch (e) {
            logError(this, `Failed to backup password:`, e);
          }

          let newComment;

          if (response.rpcResult !== null) {
            const { password } = response.rpcResult;

            try {
              fs.writeFileSync("Data/Platform/Distribution/password", password);
              newComment = 'исправлено';
            }
            catch (e) {
              logError(this, `Failed to write password:`, e);
              newComment = 'не удалось';
            }
          }
          else {
            newComment = 'не удалось';
          }

          browserState.comment = newComment;
          this.sp.browser.executeJavaScript(new FunctionInfo(this.browsersideWidgetSetter).getText({ events, browserState, authData: authData }));
        });
        break;
      case events.backToLogin:
        this.sp.browser.executeJavaScript(new FunctionInfo(this.browsersideWidgetSetter).getText({ events, browserState, authData: authData }));
        break;
      case events.joinDiscord:
        this.sp.win32.loadUrl("https://discord.gg/9KhSZ6zjGT");
        break;
      default:
        logError(this, `Unknown event key`, eventKey);
        break;
    }
  }

  private createPlaySession(token: string, callback: (res: string, err: string) => void) {
    const settingsService = this.controller.lookupListener(SettingsService);
    const client = new this.sp.HttpClient(settingsService.getMasterUrl());

    const route = `/api/users/me/play/${settingsService.getServerMasterKey()}`;
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
      } else {
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

    const settingsService = this.controller.lookupListener(SettingsService);
    const timersService = this.controller.lookupListener(TimersService);

    new this.sp.HttpClient(settingsService.getMasterUrl())
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
                  timersService.setTimeout(() => this.checkLoginState(), Math.floor((1.5 + Math.random() * 2) * 1000));
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
              browserState.comment = '';//(`Still waiting...`);
              timersService.setTimeout(() => this.checkLoginState(), Math.floor((1.5 + Math.random() * 2) * 1000));
              break;
            case 403: // Forbidden
            case 404: // Not found
              browserState.failCount = 9000;
              browserState.comment = (`Fail: ${response.body}`);
              break;
            default:
              ++browserState.failCount;
              browserState.comment = `Server returned ${response.status.toString() || "???"} "${response.body || response.error}"`;
              timersService.setTimeout(() => this.checkLoginState(), Math.floor((1.5 + Math.random() * 2) * 1000));
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

  public readAuthDataFromDisk(): RemoteAuthGameData | null {
    logTrace(this, `Reading`, this.pluginAuthDataName, `from disk`);

    try {
      // @ts-expect-error (TODO: Remove in 2.10.0)
      const data = this.sp.getPluginSourceCode(this.pluginAuthDataName, "PluginsNoLoad");

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
        content,
        // @ts-expect-error (TODO: Remove in 2.10.0)
        "PluginsNoLoad"
      );
    }
    catch (e) {
      logError(this, `Error writing`, this.pluginAuthDataName, `to disk:`, e, `, will not remember user`);
    }
  };

  private updateRequiredWidgetSetter = () => {
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
          text: "спешите скачать на",
          tags: []
        },
        {
          type: "text",
          text: "skymp.net",
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

  private updateSkipWidgetSetter = () => {
    const widget = {
      type: "form",
      id: 2,
      caption: "упс",
      elements: [
        {
          type: "text",
          text: "password отличается",
          tags: []
        },
        {
          type: "button",
          text: "исправить",
          tags: ["ELEMENT_STYLE_MARGIN_EXTENDED"],
          click: () => window.skyrimPlatform.sendMessage(events.updateSkip),
          hint: "Подменить password на актуальный",
        }
      ]
    }
    window.skyrimPlatform.widgets.set([widget]);
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
      caption: "упс",
      elements: new Array<any>()
    }

    textElements.forEach((element) => widget.elements.push(element));

    if (browserState.loginFailedReason === 'вступите в discord сервер') {
      widget.elements.push({
        type: "button",
        text: "вступить",
        tags: ["ELEMENT_STYLE_MARGIN_EXTENDED"],
        click: () => window.skyrimPlatform.sendMessage(events.joinDiscord),
        hint: null
      });
    }

    widget.elements.push({
      type: "button",
      text: "назад",
      tags: ["ELEMENT_STYLE_MARGIN_EXTENDED"],
      click: () => window.skyrimPlatform.sendMessage(events.backToLogin),
      hint: undefined
    });

    window.skyrimPlatform.widgets.set([widget]);
  }

  private browsersideWidgetSetter = () => {
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
    window.skyrimPlatform.widgets.set([loginWidget]);
  };

  private handleConnectionDenied(e: ConnectionDenied) {
    this.authAttemptProgressIndicator = false;

    if (e.error.toLowerCase().includes("invalid password")) {
      logTrace(this, 'Invalid password received');

      const settingsService = this.controller.lookupListener(SettingsService);

      const serverMasterKey = settingsService.getServerMasterKey();

      if (typeof serverMasterKey === "string" && serverMasterKey.includes("indev")) {
        logTrace(this, 'Skipping update skip required widget');

        this.controller.once("tick", () => {
          this.controller.lookupListener(NetworkingService).close();
        });

        this.sp.browser.executeJavaScript(new FunctionInfo(this.updateSkipWidgetSetter).getText({ events }));
        this.sp.browser.setVisible(true);
        this.sp.browser.setFocused(true);

      } else {
        logTrace(this, 'Showing update required widget');

        this.controller.once("tick", () => {
          this.controller.lookupListener(NetworkingService).close();
        });

        this.sp.browser.executeJavaScript(new FunctionInfo(this.updateRequiredWidgetSetter).getText({ events }));
        this.sp.browser.setVisible(true);
        this.sp.browser.setFocused(true);
        this.controller.once("update", () => {
          this.sp.Game.disablePlayerControls(true, true, true, true, true, true, true, true, 0);
        });
        this.isListenBrowserMessage = true;
      }
    }
    else {
      logError(this, 'Unknown reason for connection denied:', e.error);
    }
  }

  private handleConnectionAccepted() {
    this.isListenBrowserMessage = false;
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
      logTrace(this, 'Max logging delay reached received');

      if (this.playerEverSawActualGameplay) {
        logTrace(this, 'Player saw actual gameplay, reconnecting');
        this.loggingStartMoment = 0;
        this.controller.lookupListener(NetworkingService).reconnect();
        // TODO: should we prompt user to relogin?
      }
      else {
        logTrace(this, 'Player never saw actual gameplay, showing login dialog');
        this.loggingStartMoment = 0;
        this.authAttemptProgressIndicator = false;
        this.controller.lookupListener(NetworkingService).close();
        browserState.comment = "";
        browserState.loginFailedReason = 'технические шоколадки\nпопробуйте еще раз\nпожалуйста\nили напишите нам в discord';
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData }));

        authData = null;
        this.writeAuthDataToDisk(null);
      }
    }

    if (this.authAttemptProgressIndicator) {
      this.authAttemptProgressIndicatorCounter++;

      if (this.authAttemptProgressIndicatorCounter === 1000000) {
        this.authAttemptProgressIndicatorCounter = 0;
      }

      const slowCounter = Math.floor(this.authAttemptProgressIndicatorCounter / 15);

      const dot = slowCounter % 3 === 0 ? '.' : slowCounter % 3 === 1 ? '..' : '...';

      browserState.comment = "подключение" + dot;
      this.refreshWidgets();
    }
  }

  private onceUpdate() {
    this.playerEverSawActualGameplay = true;
  }

  // private showConnectionError() {
  //   // TODO: unhardcode it or render via browser
  //   this.sp.printConsole("Server connection failed. This may be caused by one of the following:");
  //   this.sp.printConsole("1. You are not present on the SkyMP Discord server");
  //   this.sp.printConsole("2. You have been banned by server admins");
  //   this.sp.printConsole("3. There is some technical issue. Try linking your Discord account again");
  //   this.sp.printConsole("If you feel that something is wrong, please contact us on Discord.");
  // };

  private isListenBrowserMessage = false;
  private trigger = {
    authNeededFired: false,
    browserWindowLoadedFired: false,

    get conditionMet() {
      return this.authNeededFired && this.browserWindowLoadedFired
    }
  };
  private discordAuthState = crypto.randomBytes(32).toString('hex');
  private authDialogOpen = false;

  private loggingStartMoment = 0;

  private authAttemptProgressIndicator = false;
  private authAttemptProgressIndicatorCounter = 0;

  private playerEverSawActualGameplay = false;

  private readonly githubUrl = "https://github.com/skyrim-multiplayer/skymp";
  private readonly patreonUrl = "https://www.patreon.com/skymp";
  private readonly pluginAuthDataName = `auth-data-no-load`;
}
