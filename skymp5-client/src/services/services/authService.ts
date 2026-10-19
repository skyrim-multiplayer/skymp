import * as crypto from 'node:crypto';
import * as fs from 'node:fs';
import { AuthGameData, RemoteAuthGameData, authGameDataStorageKey } from "../../features/authModel";
import { FunctionInfo } from "../../lib/functionInfo";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { BrowserMessageEvent, browser } from 'skyrimPlatform';
import { AuthNeededEvent } from "../events/authNeededEvent";
import { BrowserWindowLoadedEvent } from "../events/browserWindowLoadedEvent";
import { TimersService } from "./timersService";
import { MasterApiAuthStatus } from "../messages_http/masterApiAuthStatus";
import { logTrace, logError } from "../../logging";
import { ConnectionMessage } from "../events/connectionMessage";
import { CreateActorMessage } from "../messages/createActorMessage";
import { CustomPacketMessage } from "../messages/customPacketMessage";
import { NetworkingService } from "./networkingService";
import { MsgType } from "../../messages";
import { ConnectionDenied } from "../events/connectionDenied";
import { SettingsService } from "./settingsService";
import { FormView } from "../../view/formView";

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
  joinDiscord: 'joinDiscord',
  selectServer: 'selectServer',
  useConfiguredServer: 'useConfiguredServer',
};

interface AvailableServer {
  key: string;
  name: string;
  online: number;
  maxPlayers: number;
}

// Vaiables used on both client and browser side (see browsersideWidgetSetter)
let browserState = {
  comment: '',
  failCount: 9000,
  loginFailedReason: '',
};
let authData: RemoteAuthGameData | null = null;
let availableServers: AvailableServer[] = [];

// Variables injected at runtime via FunctionInfo.getText() into browsersideWidgetSetter
declare const selectedServerMasterKey: string;
declare const uiText: Record<string, string>;

const translations = {
  "ru": {
    loginViaDiscord: 'войдите через discord',
    joinDiscordServer: 'вступите в discord сервер',
    banned: 'вы забанены',
    whatWasThat: 'что это было?',
    openingBrowser: 'открываем браузер...',
    loginFirst: 'сначала войдите',
    linkedSuccessfully: 'привязан успешно',
    connecting: 'подключение',
    technicalIssues: 'технические шоколадки\nпопробуйте еще раз\nпожалуйста\nили напишите нам в discord',
    authorization: 'Авторизация',
    notAuthorized: 'не авторизирован',
    changeAccount: 'сменить аккаунт',
    loginViaSkymp: 'войти через skymp',
    play: 'Играть',
    loginOrChangeHint: 'Вы можете войти или поменять аккаунт',
    connectToServer: 'Подключиться к игровому серверу',
    updateCaption: 'новинка',
    updateAvailable: 'ура! вышло обновление',
    downloadAt: 'спешите скачать на',
    openSkympNet: 'открыть skymp.net',
    updateDownloadHint: 'Перейти на страницу скачивания обновления',
    oops: 'упс',
    join: 'вступить',
    back: 'назад',
  },
  "en": {
    loginViaDiscord: 'log in via Discord',
    joinDiscordServer: 'join the Discord server',
    banned: 'you are banned',
    whatWasThat: 'what was that?',
    openingBrowser: 'opening browser...',
    loginFirst: 'log in first',
    linkedSuccessfully: 'linked successfully',
    connecting: 'connecting',
    technicalIssues: 'technical difficulties\nplease try again\nor contact us on Discord',
    authorization: 'Authorization',
    notAuthorized: 'not authorized',
    changeAccount: 'change account',
    loginViaSkymp: 'log in via skymp',
    play: 'Play',
    loginOrChangeHint: 'You can log in or change your account',
    connectToServer: 'Connect to game server',
    updateCaption: 'Update',
    updateAvailable: 'a new update is available!',
    downloadAt: 'download it at',
    openSkympNet: 'open skymp.net',
    updateDownloadHint: 'Go to the update download page',
    oops: 'oops',
    join: 'join',
    back: 'back',
  },
} as const;

type TranslationStrings = { [K in keyof typeof translations['ru']]: string };

let strings: TranslationStrings = translations['en'];

try {
  const lang = fs.readFileSync('./Data/Platform/Distribution/locale', 'utf8').trim();
  if (lang in translations) {
    strings = translations[lang as keyof typeof translations];
    const src = `window.setLanguage(${lang})`;
    browser.executeJavaScript(src);
  }
} catch {
  // locale file not found or unreadable, default to 'en'
}

export class AuthService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    this.controller.emitter.on('authNeeded', (e) => this.onAuthNeeded(e));
    this.controller.emitter.on('browserWindowLoaded', (e) =>
      this.onBrowserWindowLoaded(e),
    );
    this.controller.emitter.on('createActorMessage', (e) =>
      this.onCreateActorMessage(e),
    );
    this.controller.emitter.on('connectionAccepted', () =>
      this.handleConnectionAccepted(),
    );
    this.controller.emitter.on('connectionDenied', (e) =>
      this.handleConnectionDenied(e),
    );
    this.controller.emitter.on('customPacketMessage', (e) =>
      this.onCustomPacketMessage(e),
    );
    this.controller.on('browserMessage', (e) => this.onBrowserMessage(e));
    this.controller.on('tick', () => this.onTick());
    this.controller.once('update', () => this.onceUpdate());
  }

  private onAuthNeeded(e: AuthNeededEvent) {
    logTrace(this, `Received authNeeded event`);

    const settingsGameData = this.sp.settings['skymp5-client'][
      'gameData'
    ] as any;
    const isOfflineMode = Number.isInteger(settingsGameData?.profileId);
    if (isOfflineMode) {
      logTrace(
        this,
        `Offline mode detected in settings, emitting auth event with authGameData.local`,
      );
      this.controller.emitter.emit('authAttempt', {
        authGameData: { local: { profileId: settingsGameData.profileId } },
      });
    } else {
      logTrace(
        this,
        `No offline mode detectted in settings, regular auth needed`,
      );
      this.setListenBrowserMessage(true, 'authNeeded event received');

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
        logTrace(
          this,
          `Received createActorMessage for self, resetting widgets`,
        );
        this.sp.browser.executeJavaScript(
          'window.skyrimPlatform.widgets.set([]);',
        );
        this.authDialogOpen = false;
      } else {
        logTrace(
          this,
          `Received createActorMessage for self, but auth dialog was not open so not resetting widgets`,
        );
      }
    }

    this.loggingStartMoment = 0;
    this.authAttemptProgressIndicator = false;
  }

  private onCustomPacketMessage(
    event: ConnectionMessage<CustomPacketMessage>,
  ): void {
    const msg = event.message;

    let msgContent: Record<string, unknown> = {};

    try {
      msgContent = JSON.parse(msg.contentJsonDump);
    } catch (e) {
      if (e instanceof SyntaxError) {
        logError(
          this,
          'onCustomPacketMessage failed to parse JSON',
          e.message,
          'json:',
          msg.contentJsonDump,
        );
        return;
      } else {
        throw e;
      }
    }

    switch (msgContent['customPacketType']) {
      // case 'loginRequired':
      //   logTrace(this, 'loginRequired received');
      //   this.loginWithSkympIoCredentials();
      //   break;
      case 'nicknameVisibility': {
        const enabled = Boolean(msgContent['enabled']);
        FormView.isDisplayingNicknames = enabled;
        FormView.isNicknameDisplayServerControlled = true;
        break;
      }
      case 'announcementToast': {
        const message = String(msgContent['message'] ?? '').trim();
        const durationMsRaw = Number(msgContent['durationMs']);
        if (!message) {
          break;
        }

        this.showAnnouncementToast(
          message,
          Number.isFinite(durationMsRaw) ? durationMsRaw : 4500,
        );
        break;
      }
      case 'loginFailedNotLoggedViaDiscord':
        this.authAttemptProgressIndicator = false;
        this.controller.lookupListener(NetworkingService).close();
        logTrace(this, 'loginFailedNotLoggedViaDiscord received');
        browserState.loginFailedReason = strings.loginViaDiscord;
        browserState.comment = '';
        this.setListenBrowserMessage(
          true,
          'loginFailedNotLoggedViaDiscord received',
        );
        this.loggingStartMoment = 0;
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData, strings }));
        break;
      case 'loginFailedNotInTheDiscordServer':
        this.authAttemptProgressIndicator = false;
        this.controller.lookupListener(NetworkingService).close();
        logTrace(this, 'loginFailedNotInTheDiscordServer received');
        browserState.loginFailedReason = strings.joinDiscordServer;
        browserState.comment = '';
        this.setListenBrowserMessage(
          true,
          'loginFailedNotInTheDiscordServer received',
        );
        this.loggingStartMoment = 0;
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData, strings }));
        break;
      case 'loginFailedBanned':
        this.authAttemptProgressIndicator = false;
        this.controller.lookupListener(NetworkingService).close();
        logTrace(this, 'loginFailedBanned received');
        browserState.loginFailedReason = strings.banned;
        browserState.comment = '';
        this.setListenBrowserMessage(true, 'loginFailedBanned received');
        this.loggingStartMoment = 0;
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData, strings }));
        break;
      case 'loginFailedIpMismatch':
        this.authAttemptProgressIndicator = false;
        this.controller.lookupListener(NetworkingService).close();
        logTrace(this, 'loginFailedIpMismatch received');
        browserState.loginFailedReason = strings.whatWasThat;
        browserState.comment = '';
        this.setListenBrowserMessage(true, 'loginFailedIpMismatch received');
        this.loggingStartMoment = 0;
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData, strings }));
        break;
    }
  }

  private onBrowserWindowLoadedAndOnlineAuthNeeded() {
    if (!this.isListenBrowserMessage) {
      logError(
        this,
        `isListenBrowserMessage was false for some reason, aborting auth`,
      );
      return;
    }

    logTrace(this, `Showing widgets and starting loop`);

    authData = this.readAuthDataFromDisk();
    this.loadAvailableServers();
    this.sp.browser.executeJavaScript(
      `window.setLanguage('${this.getCurrentLanguage()}')`,
    );
    this.refreshWidgets();
    this.sp.browser.setVisible(true);
    this.sp.browser.setFocused(true);

    const timersService = this.controller.lookupListener(TimersService);

    logTrace(this, 'Calling setTimeout for testing');
    try {
      timersService.setTimeout(() => {
        logTrace(this, 'Test timeout fired');
      }, 1);
    } catch (e) {
      logError(this, 'Failed to call setTimeout');
    }
  }

  private onBrowserMessage(e: BrowserMessageEvent) {
    if (!this.isListenBrowserMessage) {
      logTrace(
        this,
        `onBrowserMessage: isListenBrowserMessage was false, ignoring message`,
        JSON.stringify(e.arguments),
      );
      return;
    }

    const settingsService = this.controller.lookupListener(SettingsService);

    logTrace(this, `onBrowserMessage:`, JSON.stringify(e.arguments));

    const eventKey = e.arguments[0];
    const currentTexts = this.getCurrentTexts();
    switch (eventKey) {
      case events.openDiscordOauth:
        browserState.comment = strings.openingBrowser;
        this.refreshWidgets();
        this.sp.win32.loadUrl(
          `${settingsService.getMasterUrl()}/api/users/login-discord?state=${
            this.discordAuthState
          }`,
        );

        // Launch checkLoginState loop
        this.checkLoginState();
        break;
      case events.authAttempt:
        if (authData === null) {
          browserState.comment = strings.loginFirst;
          this.refreshWidgets();
          break;
        }

        this.writeAuthDataToDisk(authData);
        this.controller.emitter.emit('authAttempt', {
          authGameData: { remote: authData },
        });

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
        this.sp.win32.loadUrl('https://skymp.net/UpdInstall');
        break;
      case events.backToLogin:
        this.sp.browser.executeJavaScript(new FunctionInfo(this.browsersideWidgetSetter).getText({ events, browserState, authData: authData, strings }));
        break;
      case events.joinDiscord:
        this.sp.win32.loadUrl('https://discord.gg/9KhSZ6zjGT');
        break;
      case events.selectServer:
        {
          const requestedServerKey = e.arguments[1];
          if (
            typeof requestedServerKey === 'string' &&
            requestedServerKey.length > 0
          ) {
            settingsService.setSelectedServerMasterKey(requestedServerKey);
            browserState.comment = `${currentTexts.selectedServer}: ${requestedServerKey}`;
            this.refreshWidgets();
          }
        }
        break;
      case events.useConfiguredServer:
        settingsService.clearSelectedServerMasterKey();
        browserState.comment = currentTexts.usingConfiguredServer;
        this.refreshWidgets();
        break;
      default:
        break;
    }
  }

  private createPlaySession(
    token: string,
    callback: (res: string, err: string) => void,
  ) {
    const settingsService = this.controller.lookupListener(SettingsService);
    const client = new this.sp.HttpClient(settingsService.getMasterUrl());

    const route = `/api/users/me/play/${settingsService.getServerMasterKey()}`;
    logTrace(this, `Creating play session ${route}`);

    client
      .post(route, {
        body: '{}',
        contentType: 'application/json',
        headers: {
          authorization: token,
        },
      })
      .then((res) => {
        if (res.status != 200) {
          callback('', 'status code ' + res.status);
        } else {
          // TODO: handle JSON.parse failure?
          callback(JSON.parse(res.body).session, '');
        }
      })
      .catch((err) => {
        callback('', String(err));
      });
  }

  private checkLoginState() {
    if (!this.isListenBrowserMessage) {
      logTrace(
        this,
        `checkLoginState: isListenBrowserMessage was false, aborting check`,
      );
      return;
    }

    const settingsService = this.controller.lookupListener(SettingsService);
    const timersService = this.controller.lookupListener(TimersService);

    // Social engineering protection, don't show the full state
    const halfDiscordAuthState = this.discordAuthState.slice(0, 16);

    logTrace(this, `Checking login state`, halfDiscordAuthState, '...');

    new this.sp.HttpClient(settingsService.getMasterUrl()).get(
      '/api/users/login-discord/status?state=' + this.discordAuthState,
      undefined,
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
                browserState.comment = error;
                timersService.setTimeout(
                  () => this.checkLoginState(),
                  Math.floor((1.5 + Math.random() * 2) * 1000),
                );
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
              browserState.comment = strings.linkedSuccessfully;
              this.refreshWidgets();
            });
            break;
          case 401: // Unauthorized
            browserState.failCount = 0;
            browserState.comment = '';
            timersService.setTimeout(
              () => this.checkLoginState(),
              Math.floor((1.5 + Math.random() * 2) * 1000),
            );
            break;
          case 403: // Forbidden
          case 404: // Not found
            browserState.failCount = 9000;
            browserState.comment = `Fail: ${response.body}`;
            break;
          default:
            ++browserState.failCount;
            browserState.comment = `Server returned ${response.status.toString() || "???"} "${response.body || response.error}"`;
            timersService.setTimeout(
              () => this.checkLoginState(),
              Math.floor((1.5 + Math.random() * 2) * 1000),
            );
        }
      },
    );
  }

  private loadAvailableServers(): void {
    const settingsService = this.controller.lookupListener(SettingsService);
    const masterApiClient = settingsService.makeMasterApiClient();

    masterApiClient.get(
      '/api/servers',
      undefined,
      // @ts-ignore
      (res) => {
        if (res.status !== 200) {
          logTrace(this, `Failed to load server list, status ${res.status}`);
          return;
        }

        try {
          const data = JSON.parse(res.body);
          if (!Array.isArray(data)) {
            return;
          }

          const mapped = data
            .map((item: Record<string, unknown>) => {
              const key =
                item.masterKey || item.key || item.serverKey || item.id;
              const keyStr = typeof key === 'string' ? key : '';
              if (!keyStr) {
                return null;
              }

              const name = item.name;
              const online = item.online;
              const maxPlayers = item.maxPlayers;

              return {
                key: keyStr,
                name:
                  typeof name === 'string' && name.length > 0 ? name : keyStr,
                online: typeof online === 'number' ? online : 0,
                maxPlayers: typeof maxPlayers === 'number' ? maxPlayers : 0,
              } as AvailableServer;
            })
            .filter(
              (server: AvailableServer | null): server is AvailableServer =>
                !!server,
            )
            .sort((a: AvailableServer, b: AvailableServer) => b.online - a.online)
            .slice(0, 50);

          availableServers = mapped;
          this.refreshWidgets();
        } catch (e) {
          logError(this, `Failed to parse server list`, e);
        }
      },
    );
  }

  private refreshWidgets() {
    const currentTexts = this.getCurrentTexts();
    this.sp.browser.executeJavaScript(new FunctionInfo(this.browsersideWidgetSetter).getText({ events, browserState, authData: authData, availableServers, selectedServerMasterKey: this.controller.lookupListener(SettingsService).getServerMasterKey(), uiText: currentTexts }));
    this.authDialogOpen = true;
  }

  public readAuthDataFromDisk(): RemoteAuthGameData | null {
    logTrace(this, `Reading`, this.pluginAuthDataName, `from disk`);

    try {
      const data = this.sp.getPluginSourceCode(this.pluginAuthDataName);

      if (!data) {
        logTrace(this, `Read empty`, this.pluginAuthDataName, `returning null`);
        return null;
      }

      return JSON.parse(data.slice(2)) || null;
    } catch (e) {
      logError(
        this,
        `Error reading`,
        this.pluginAuthDataName,
        `from disk:`,
        e,
        `, falling back to null`,
      );
      return null;
    }
  }

  private writeAuthDataToDisk(data: RemoteAuthGameData | null) {
    const content = '//' + (data ? JSON.stringify(data) : 'null');

    logTrace(this, `Writing`, this.pluginAuthDataName, `to disk:`, content);

    try {
      this.sp.writePlugin(
        this.pluginAuthDataName,
        content,
      );
    } catch (e) {
      logError(
        this,
        `Error writing`,
        this.pluginAuthDataName,
        `to disk:`,
        e,
        `, will not remember user`,
      );
    }
  }

  private deniedWidgetSetter = () => {
    const widget = {
      type: 'form',
      id: 2,
      caption: strings.updateCaption,
      elements: [
        {
          type: "text",
          text: strings.updateAvailable,
          tags: []
        },
        {
          type: "text",
          text: strings.downloadAt,
          tags: []
        },
        {
          type: 'text',
          text: 'skymp.net',
          tags: [],
        },
        {
          type: "button",
          text: strings.openSkympNet,
          tags: ["ELEMENT_STYLE_MARGIN_EXTENDED"],
          click: () => window.skyrimPlatform.sendMessage(events.updateRequired),
          hint: strings.updateDownloadHint,
        }
      ]
    }
    window.skyrimPlatform.widgets.set([widget]);

    // Make sure gamemode will not be able to update widgets anymore
    window.skyrimPlatform.widgets = null;
  };

  private loginFailedWidgetSetter = () => {
    const splitParts = browserState.loginFailedReason.split('\n');

    const textElements = splitParts.map((part) => ({
      type: 'text',
      text: part,
      tags: [],
    }));

    const widget = {
      type: 'form',
      id: 2,
      caption: strings.oops,
      elements: new Array<any>()
    }

    textElements.forEach((element) => widget.elements.push(element));

    if (browserState.loginFailedReason === strings.joinDiscordServer) {
      widget.elements.push({
        type: "button",
        text: strings.join,
        tags: ["ELEMENT_STYLE_MARGIN_EXTENDED"],
        click: () => window.skyrimPlatform.sendMessage(events.joinDiscord),
        hint: null,
      });
    }

    widget.elements.push({
      type: "button",
      text: strings.back,
      tags: ["ELEMENT_STYLE_MARGIN_EXTENDED"],
      click: () => window.skyrimPlatform.sendMessage(events.backToLogin),
      hint: undefined,
    });

    window.skyrimPlatform.widgets.set([widget]);
  };

  private browsersideWidgetSetter = () => {
    const serverButtons = (availableServers || []).map((server) => ({
      type: 'button',
      text: `${selectedServerMasterKey === server.key ? '[x]' : '[ ]'} ${
        server.name
      } (${server.online}/${server.maxPlayers})`,
      tags: ['ELEMENT_STYLE_MARGIN_EXTENDED'],
      click: () =>
        window.skyrimPlatform.sendMessage(events.selectServer, server.key),
      hint: `${uiText.serverKey}: ${server.key}`,
    }));

    const loginWidget = {
      type: 'form',
      id: 1,
      caption: uiText.authorization,
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
            ) : uiText.notAuthorized
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
          text: authData ? uiText.switchAccount : uiText.loginViaSkymp,
          tags: [/*"ELEMENT_SAME_LINE"*/],
          click: () => window.skyrimPlatform.sendMessage(events.openDiscordOauth),
          hint: uiText.switchAccountHint,
        },
        {
          type: 'text',
          text: `${uiText.currentServer}: ${selectedServerMasterKey}`,
          tags: ['ELEMENT_STYLE_MARGIN_EXTENDED'],
        },
        {
          type: 'button',
          text: uiText.useConfiguredServer,
          tags: ['ELEMENT_STYLE_MARGIN_EXTENDED'],
          click: () => window.skyrimPlatform.sendMessage(events.useConfiguredServer),
          hint: uiText.useConfiguredServerHint,
        },
        {
          type: 'text',
          text: uiText.serverList,
          tags: [],
        },
        ...serverButtons,
        {
          type: "button",
          text: uiText.play,
          tags: ["BUTTON_STYLE_FRAME", "ELEMENT_STYLE_MARGIN_EXTENDED"],
          click: () => window.skyrimPlatform.sendMessage(events.authAttempt),
          hint: uiText.playHint,
        },
        {
          type: 'text',
          text: browserState.comment,
          tags: [],
        },
      ],
    };
    window.skyrimPlatform.widgets.set([loginWidget]);
  };

  private handleConnectionDenied(e: ConnectionDenied) {
    this.authAttemptProgressIndicator = false;

    if (e.error.toLowerCase().includes('invalid password')) {
      this.controller.once('tick', () => {
        this.controller.lookupListener(NetworkingService).close();
      });
      this.sp.browser.executeJavaScript(new FunctionInfo(this.deniedWidgetSetter).getText({ events, strings }));
      this.sp.browser.setVisible(true);
      this.sp.browser.setFocused(true);
      this.controller.once('update', () => {
        this.sp.Game.disablePlayerControls(
          true,
          true,
          true,
          true,
          true,
          true,
          true,
          true,
          0,
        );
      });
      this.setListenBrowserMessage(true, 'connectionDenied event received');
    }
  }

  private handleConnectionAccepted() {
    this.setListenBrowserMessage(false, 'connectionAccepted event received');
    this.loggingStartMoment = Date.now();

    const authData = this.sp.storage[authGameDataStorageKey] as
      | AuthGameData
      | undefined;
    if (authData?.local) {
      logTrace(
        this,
        `Logging in offline mode, profileId =`,
        authData.local.profileId,
      );
      const message: CustomPacketMessage = {
        t: MsgType.CustomPacket,
        contentJsonDump: JSON.stringify({
          customPacketType: 'loginWithSkympIo',
          gameData: {
            profileId: authData.local.profileId,
          },
        }),
      };
      this.controller.emitter.emit('sendMessage', {
        message: message,
        reliability: 'reliable',
      });
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
      this.controller.emitter.emit('sendMessage', {
        message: message,
        reliability: 'reliable',
      });
      return;
    }

    logError(this, 'Not found authentication method');
  }

  private onTick() {
    // TODO: Should be no hardcoded/magic-number limit
    // TODO: Busy waiting is bad. Should be replaced with some kind of event
    const maxLoggingDelay = 15000;
    if (
      this.loggingStartMoment &&
      Date.now() - this.loggingStartMoment > maxLoggingDelay
    ) {
      logTrace(this, 'Max logging delay reached received');

      if (this.playerEverSawActualGameplay) {
        logTrace(this, 'Player saw actual gameplay, reconnecting');
        this.loggingStartMoment = 0;
        this.controller.lookupListener(NetworkingService).reconnect();
        // TODO: should we prompt user to relogin?
      } else {
        logTrace(
          this,
          'Player never saw actual gameplay, showing login dialog',
        );
        this.loggingStartMoment = 0;
        this.authAttemptProgressIndicator = false;
        this.controller.lookupListener(NetworkingService).close();
        browserState.comment = "";
        browserState.loginFailedReason = strings.technicalIssues;
        this.sp.browser.executeJavaScript(new FunctionInfo(this.loginFailedWidgetSetter).getText({ events, browserState, authData: authData, strings }));

        authData = null;
        this.writeAuthDataToDisk(null);
      }
    }

    if (this.authAttemptProgressIndicator) {
      this.authAttemptProgressIndicatorCounter++;

      if (this.authAttemptProgressIndicatorCounter === 1000000) {
        this.authAttemptProgressIndicatorCounter = 0;
      }

      const slowCounter = Math.floor(
        this.authAttemptProgressIndicatorCounter / 15,
      );

      const dot =
        slowCounter % 3 === 0 ? '.' : slowCounter % 3 === 1 ? '..' : '...';

      browserState.comment = strings.connecting + dot;
      this.refreshWidgets();
    }
  }

  private getCurrentLanguage(): 'ru' | 'en' | 'de' {
    const lang = `${
      this.sp.settings['skymp5-client']['lang'] ||
      this.sp.settings['skymp5-client']['language'] ||
      'ru'
    }`.toLowerCase();
    if (lang.startsWith('de')) {
      return 'de';
    }
    if (lang.startsWith('en')) {
      return 'en';
    }
    return 'ru';
  }

  private getCurrentTexts() {
    return this.uiText[this.getCurrentLanguage()];
  }

  private onceUpdate() {
    this.playerEverSawActualGameplay = true;
  }

  // Render server announcements as a small animated HUD banner at the top of
  // the browser overlay instead of reusing chat.
  private showAnnouncementToast(message: string, durationMs: number): void {
    const safeMessage = JSON.stringify(String(message || '').slice(0, 240));
    const safeDuration = Math.max(1500, Math.min(15000, Math.floor(durationMs)));

    const script = `(() => {
      const message = ${safeMessage};
      const durationMs = ${safeDuration};
      const rootId = 'skymp-announcement-toast-root';
      const styleId = 'skymp-announcement-toast-style';

      const ensureStyle = () => {
        if (document.getElementById(styleId)) return;
        const style = document.createElement('style');
        style.id = styleId;
        style.textContent = ${JSON.stringify(`
          #skymp-announcement-toast-root {
            position: fixed;
            top: 22px;
            left: 50%;
            transform: translateX(-50%);
            z-index: 2147483647;
            pointer-events: none;
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 8px;
            width: min(820px, calc(100vw - 32px));
          }
          .skymp-announcement-toast {
            box-sizing: border-box;
            width: 100%;
            padding: 14px 20px;
            border-radius: 12px;
            border: 1px solid rgba(197, 167, 88, 0.78);
            background:
              linear-gradient(180deg, rgba(43, 31, 14, 0.94), rgba(13, 10, 7, 0.96)),
              radial-gradient(circle at top, rgba(255, 227, 162, 0.16), transparent 58%);
            color: #f4d98b;
            box-shadow:
              0 0 0 1px rgba(0, 0, 0, 0.35) inset,
              0 16px 40px rgba(0, 0, 0, 0.58),
              0 0 24px rgba(213, 176, 84, 0.18);
            font-family: Georgia, 'Times New Roman', serif;
            font-size: 17px;
            font-weight: 700;
            line-height: 1.35;
            letter-spacing: 0.02em;
            text-align: center;
            opacity: 0;
            transform: translateY(-34px) scale(0.985);
            transition:
              opacity 240ms ease,
              transform 240ms cubic-bezier(0.2, 0.9, 0.2, 1);
            will-change: opacity, transform;
            position: relative;
          }
          .skymp-announcement-toast::before,
          .skymp-announcement-toast::after {
            content: '';
            position: absolute;
            left: 12px;
            right: 12px;
            height: 1px;
            background: linear-gradient(
              90deg,
              transparent,
              rgba(255, 221, 146, 0.72),
              transparent
            );
          }
          .skymp-announcement-toast::before {
            top: 8px;
          }
          .skymp-announcement-toast::after {
            bottom: 8px;
          }
          .skymp-announcement-toast.is-visible {
            opacity: 1;
            transform: translateY(0);
          }
        `)};
        document.head.appendChild(style);
      };

      const ensureRoot = () => {
        ensureStyle();
        let root = document.getElementById(rootId);
        if (!root) {
          root = document.createElement('div');
          root.id = rootId;
          document.body.appendChild(root);
        }
        return root;
      };

      const root = ensureRoot();
      const toast = document.createElement('div');
      toast.className = 'skymp-announcement-toast';
      toast.textContent = message;
      root.appendChild(toast);

      requestAnimationFrame(() => {
        toast.classList.add('is-visible');
      });

      window.setTimeout(() => {
        toast.classList.remove('is-visible');
        window.setTimeout(() => {
          if (toast.parentNode) {
            toast.parentNode.removeChild(toast);
          }
        }, 260);
      }, durationMs);
    })();`;

    try {
      this.sp.browser.executeJavaScript(script);
    } catch (error) {
      logError(this, 'Failed to show announcement toast', error);
    }
  }

  private get isListenBrowserMessage() {
    return this._isListenBrowserMessage;
  }

  private setListenBrowserMessage(value: boolean, reason: string) {
    logTrace(this, `setListenBrowserMessage:`, value, `reason:`, reason);
    this._isListenBrowserMessage = value;
  }

  private _isListenBrowserMessage = false;

  private trigger = {
    authNeededFired: false,
    browserWindowLoadedFired: false,

    get conditionMet() {
      return this.authNeededFired && this.browserWindowLoadedFired;
    },
  };
  private discordAuthState = crypto.randomBytes(32).toString('hex');
  private authDialogOpen = false;

  private loggingStartMoment = 0;

  private authAttemptProgressIndicator = false;
  private authAttemptProgressIndicatorCounter = 0;

  private playerEverSawActualGameplay = false;

  private readonly githubUrl = 'https://github.com/skyrim-multiplayer/skymp';
  private readonly patreonUrl = 'https://www.patreon.com/skymp';
  private readonly pluginAuthDataName = `auth-data-no-load`;
  private readonly uiText = {
    ru: {
      authorization: 'Авторизация',
      notAuthorized: 'не авторизирован',
      switchAccount: 'сменить аккаунт',
      loginViaSkymp: 'войти через skymp',
      switchAccountHint: 'Вы можете войти или поменять аккаунт',
      currentServer: 'Текущий сервер',
      serverList: 'Список серверов',
      serverKey: 'Ключ сервера',
      selectedServer: 'Выбран сервер',
      useConfiguredServer: 'Использовать сервер из настроек',
      useConfiguredServerHint:
        'Сбросить выбранный сервер и взять server-master-key или server-ip:server-port',
      play: 'Играть',
      playHint: 'Подключиться к игровому серверу',
      openingBrowser: 'открываем браузер...',
      loginFirst: 'сначала войдите',
      linkedSuccessfully: 'привязан успешно',
      connecting: 'подключение',
      usingConfiguredServer: 'Сервер из настроек включен',
    },
    en: {
      authorization: 'Authorization',
      notAuthorized: 'not authorized',
      switchAccount: 'switch account',
      loginViaSkymp: 'sign in with skymp',
      switchAccountHint: 'You can sign in or switch account',
      currentServer: 'Current server',
      serverList: 'Server list',
      serverKey: 'Server key',
      selectedServer: 'Selected server',
      useConfiguredServer: 'Use server from config',
      useConfiguredServerHint:
        'Reset selected server and use server-master-key or server-ip:server-port',
      play: 'Play',
      playHint: 'Connect to the game server',
      openingBrowser: 'opening browser...',
      loginFirst: 'please log in first',
      linkedSuccessfully: 'linked successfully',
      connecting: 'connecting',
      usingConfiguredServer: 'Configured server is active',
    },
    de: {
      authorization: 'Anmeldung',
      notAuthorized: 'nicht angemeldet',
      switchAccount: 'Konto wechseln',
      loginViaSkymp: 'mit skymp anmelden',
      switchAccountHint: 'Du kannst dich anmelden oder das Konto wechseln',
      currentServer: 'Aktueller Server',
      serverList: 'Serverliste',
      serverKey: 'Server-Schluessel',
      selectedServer: 'Server ausgewaehlt',
      useConfiguredServer: 'Server aus Konfiguration nutzen',
      useConfiguredServerHint:
        'Ausgewaehlten Server zuruecksetzen und server-master-key oder server-ip:server-port nutzen',
      play: 'Spielen',
      playHint: 'Mit dem Spielserver verbinden',
      openingBrowser: 'Browser wird geoeffnet...',
      loginFirst: 'bitte zuerst anmelden',
      linkedSuccessfully: 'erfolgreich verknuepft',
      connecting: 'verbinde',
      usingConfiguredServer: 'Konfigurierter Server ist aktiv',
    },
  } as const;
}
