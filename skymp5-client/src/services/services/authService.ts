import { RemoteAuthGameData } from "../../features/authModel";
import { FunctionInfo } from "../../lib/functionInfo";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { BrowserMessageEvent } from "skyrimPlatform";
import { AuthNeededEvent } from "../events/authNeededEvent";
import { BrowserWindowLoadedEvent } from "../events/browserWindowLoadedEvent";
import { TimersService } from "./timersService";

interface MasterApiAuthStatus {
  token: string;
  masterApiId: number;
  discordUsername: string | null;
  discordDiscriminator: string | null;
  discordAvatar: string | null;
}

// for browsersideWidgetSetter
declare const window: any;

// Constants used on both client and browser side (see browsersideWidgetSetter)
const events = {
  openDiscordOauth: 'openDiscordOauth',
  login: 'loginRequiredEvent',
  openGithub: 'openGithub',
  openPatreon: 'openPatreon',
  clearAuthData: 'clearAuthData',
};

// Vaiables used on both client and browser side (see browsersideWidgetSetter)
let browserState = {
  comment: '',
  failCount: 9000,
};
let authData: RemoteAuthGameData | null = null;

export class AuthService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    this.controller.emitter.on("authNeeded", (e) => this.onAuthNeeded(e));
    this.controller.emitter.on("browserWindowLoaded", (e) => this.onBrowserWindowLoaded(e))

    this.controller.on("browserMessage", (e) => this.onBrowserMessage(e));
  }

  private onAuthNeeded(e: AuthNeededEvent) {
    this.logTrace(`Received authNeeded event`);

    const settingsGameData = this.sp.settings["skymp5-client"]["gameData"] as any;
    const isOfflineMode = Number.isInteger(settingsGameData?.profileId);
    if (isOfflineMode) {
      this.logTrace(`Offline mode detected in settings, emitting auth event with authGameData.local`);
      this.controller.emitter.emit("auth", { authGameData: { local: { profileId: settingsGameData.profileId } } });
    } else {
      this.logTrace(`No offline mode detectted in settings, regular auth needed`);
      this.isListenBrowserMessage = true;

      this.trigger.authNeededFired = true;
      if (this.trigger.conditionMet) {
        this.onBrowserWindowLoadedAndOnlineAuthNeeded();
      }
    }
  }

  private onBrowserWindowLoaded(e: BrowserWindowLoadedEvent) {
    this.logTrace(`Received browserWindowLoaded event`);

    this.trigger.browserWindowLoadedFired = true;
    if (this.trigger.conditionMet) {
      this.onBrowserWindowLoadedAndOnlineAuthNeeded();
    }
  }

  private onBrowserWindowLoadedAndOnlineAuthNeeded() {
    if (!this.isListenBrowserMessage) {
      this.logError(`isListenBrowserMessage was false for some reason, aborting auth`);
      return;
    }

    this.logTrace(`Showing widgets and starting loop`);

    authData = this.readAuthDataFromDisk();
    this.refreshWidgets();
    this.sp.browser.setVisible(true);
    this.sp.browser.setFocused(true);

    const timersService = this.controller.lookupListener(TimersService);

    this.logTrace("Calling setTimeout for testing");
    try {
      timersService.setTimeout(() => {
        this.logTrace("Test timeout fired");
      }, 1);
    }
    catch (e) {
      this.logError("Failed to call setTimeout");
    }

    // Launch checkLoginState loop
    this.checkLoginState();
  }

  private onBrowserMessage(e: BrowserMessageEvent) {
    if (!this.isListenBrowserMessage) {
      this.logTrace(`onBrowserMessage: isListenBrowserMessage was false, ignoring message ${JSON.stringify(e.arguments)}`);
      return;
    }

    this.logTrace(`onBrowserMessage: ${JSON.stringify(e.arguments)}`);

    const eventKey = e.arguments[0];
    switch (eventKey) {
      case events.openDiscordOauth:
        this.sp.win32.loadUrl(`${this.getMasterUrl()}/api/users/login-discord?state=${this.discordAuthState}`);
        break;
      case events.login:
        if (authData === null) {
          browserState.comment = 'logic error: remoteAuthGameData is null';
          this.refreshWidgets();
          break;
        }
        this.writeAuthDataToDisk(authData);
        this.controller.emitter.emit("auth", { authGameData: { remote: authData } });
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
      default:
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

    this.logTrace(`Creating play session ${route}`);

    client.post(route, {
      body: '{}',
      contentType: 'application/json',
      headers: {
        'authorization': token,
      },
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
      this.logTrace(`checkLoginState: isListenBrowserMessage was false, aborting check`);
      return;
    }

    const timersService = this.controller.lookupListener(TimersService);

    new this.sp.HttpClient(this.getMasterUrl())
      .get("/api/users/login-discord/status?state=" + this.discordAuthState, undefined,
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
      this.logError(`Auth check fail: ${browserState.comment}`);
    }
    this.sp.browser.executeJavaScript(new FunctionInfo(this.browsersideWidgetSetter).getText({ events, browserState, authData: authData }));
  };

  private readAuthDataFromDisk(): RemoteAuthGameData | null {
    this.logTrace(`Reading ${this.pluginAuthDataName} from disk`);

    try {
      const data = this.sp.getPluginSourceCode(this.pluginAuthDataName);

      if (!data) {
        this.logTrace(`Read empty ${this.pluginAuthDataName}, returning null`);
        return null;
      }

      return JSON.parse(data.slice(2)) || null;
    } catch (e) {
      this.logError(`Error reading ${this.pluginAuthDataName} from disk: ${e}, falling back to null`);
      return null;
    }
  }

  private writeAuthDataToDisk(data: RemoteAuthGameData | null) {
    const content = "//" + (data ? JSON.stringify(data) : "null");

    this.logTrace(`Writing ${this.pluginAuthDataName} to disk: ${content}`);

    try {
      this.sp.writePlugin(
        this.pluginAuthDataName,
        content
      );
    }
    catch (e) {
      this.logError(`Error writing ${this.pluginAuthDataName} to disk: ${e}, will not remember user`);
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
          click: () => window.skyrimPlatform.sendMessage(events.login),
          hint: "Подключиться к игровому серверу",
        },
        {
          type: "text",
          text: browserState.failCount > 3 ? browserState.comment : "",
          tags: [],
        },
      ]
    };
    console.log(loginWidget);
    window.skyrimPlatform.widgets.set([loginWidget]);
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

  private readonly githubUrl = "https://github.com/skyrim-multiplayer/skymp";
  private readonly patreonUrl = "https://www.patreon.com/skymp";
  private readonly pluginAuthDataName = `auth-data-no-load`;
}
