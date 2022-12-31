import * as sp from "skyrimPlatform";
import * as browser from "./browser";
import * as loadGameManager from "./loadGameManager";
import { AuthGameData, RemoteAuthGameData } from "./authModel";
import { Transform } from "../sync/movement";
import { FunctionInfo } from "../lib/functionInfo";

const normalizeUrl = (url: string) => {
  if (url.endsWith('/')) {
    return url.slice(0, url.length - 1);
  }
  return url;
};

const authUrl = normalizeUrl((sp.settings["skymp5-client"]["master"] as string) || "https://skymp.io");
const githubUrl = "https://github.com/skyrim-multiplayer/skymp";
const patreonUrl = "https://www.patreon.com/skymp";

const events = {
  openDiscordOauth: 'openDiscordOauth',
  login: 'loginRequiredEvent',
  openGithub: 'openGithub',
  openPatreon: 'openPatreon',
  clearAuthData: 'clearAuthData',
};

const browserState = {
  comment: '',
  failCount: 9000,
};

const discordAuthState = "" + Math.random();

interface AuthStatus {
  token: string;
  masterApiId: number;
  discordUsername: string | null;
  discordDiscriminator: string | null;
  discordAvatar: string | null;
}

let authData: RemoteAuthGameData | null = null;

export type AuthCallback = (data: AuthGameData) => void;

const authListeners = new Array<AuthCallback>();
const onAuthListeners = (data: AuthGameData): void => {
  isListenBrowserMessage = false;
  authListeners.forEach(listener => listener(data));
}
export const addAuthListener = (callback: AuthCallback): void => {
  authListeners.push(callback);
}

export const main = (lobbyLocation: Transform): void => {
  const settingsGameData = sp.settings["skymp5-client"]["gameData"] as any;
  const isOfflineMode = Number.isInteger(settingsGameData?.profileId);
  if (isOfflineMode) {
    onAuthListeners({ local: { profileId: settingsGameData.profileId } });
  } else {
    startListenBrowserMessage();
    browser.addOnWindowLoadListener(() => {
      if (isListenBrowserMessage) loadLobby(lobbyLocation);
    });
  }
}

let defaultAutoVanityModeDelay: number = 120;
export const setPlayerAuthMode = (frozen: boolean): void => {
  if (frozen) {
    sp.Utility.setINIFloat("fAutoVanityModeDelay:Camera", 72000.0);
  } else {
    sp.Utility.setINIFloat("fAutoVanityModeDelay:Camera", defaultAutoVanityModeDelay);
  }

  sp.Game.getPlayer()!.setDontMove(frozen);
  sp.Game.forceFirstPerson();
}

function createPlaySession(token: string) {
  const client = new sp.HttpClient(authUrl);
  let masterKey = sp.settings["skymp5-client"]["server-master-key"];
  if (!masterKey) {
    masterKey = sp.settings["skymp5-client"]["master-key"];
  }
  if (!masterKey) {
    masterKey = sp.settings["skymp5-client"]["server-ip"] + ":" + sp.settings["skymp5-client"]["server-port"];
  }
  sp.printConsole({ masterKey });
  return client.post(`/api/users/me/play/${masterKey}`, {
    body: '{}',
    contentType: 'application/json',
    headers: {
      'authorization': token,
    },
  }).then((res) => {
    if (res.status != 200) {
      throw Error('status code ' + res.status);
    }
    return JSON.parse(res.body).session;
  });
}

let isListenBrowserMessage = false;
const startListenBrowserMessage = (): void => {
  isListenBrowserMessage = true;
  onBrowserMessage();
}
const onBrowserMessage = (): void => {
  sp.once("browserMessage", (e) => {
    if (!isListenBrowserMessage) return;
    sp.printConsole(JSON.stringify(e.arguments));
    const eventKey = e.arguments[0];
    switch (eventKey) {
      case events.openDiscordOauth:
        sp.win32.loadUrl(`${authUrl}/api/users/login-discord?state=${discordAuthState}`);
        break;
      case events.login:
        if (!authData) {
          browserState.comment = 'logic error: remoteAuthGameData is null';
          refreshWidgets();
          break;
        }
        onAuthListeners({ remote: authData });
        break;
      case events.clearAuthData:
        browser.setAuthData(null);
        break;
      case events.openGithub:
        sp.win32.loadUrl(githubUrl);
        break;
      case events.openPatreon:
        sp.win32.loadUrl(patreonUrl);
        break;
      default:
        break;
    }
    if (isListenBrowserMessage) {
      onBrowserMessage();
    }
  })
}

const checkLoginState = () => {
  if (!isListenBrowserMessage) {
    return;
  }
  
  new sp.HttpClient(authUrl)
    .get("/api/users/login-discord/status?state=" + discordAuthState)
    .then(response => {
      switch (response.status) {
        case 200:
          const {
            token,
            masterApiId,
            discordUsername,
            discordDiscriminator,
            discordAvatar,
          } = JSON.parse(response.body) as AuthStatus;
          browserState.failCount = 0;
          createPlaySession(token).then((playSession) => {
            authData = {
              session: playSession,
              masterApiId,
              discordUsername,
              discordDiscriminator,
              discordAvatar,
            };
            refreshWidgets();
          });
          break;
        case 401: // Unauthorized
          browserState.failCount = 0;
          browserState.comment = (`Still waiting...`);
          setTimeout(() => checkLoginState(), 1.5 + Math.random() * 2);
          break;
        case 403: // Forbidden
        case 404: // Not found
          browserState.failCount = 9000;
          browserState.comment = (`Fail: ${response.body}`);
          break;
        default:
          ++browserState.failCount;
          browserState.comment = `Server returned ${response.status.toString() || "???"} "${response.body || response.error}"`;
          setTimeout(() => checkLoginState(), 1.5 + Math.random() * 2);
      }
    })
    .catch(reason => {
      ++browserState.failCount;
      if (typeof reason === "string") {
        browserState.comment = (`Skyrim platform error (http): ${reason}`)
      } else {
        browserState.comment = (`Skyrim platform error (http): request rejected`);
      }
    })
    .finally(() => {
      refreshWidgets();
    });
};

const loadLobby = (location: Transform): void => {
  sp.once("update", () => {
    defaultAutoVanityModeDelay = sp.Utility.getINIFloat("fAutoVanityModeDelay:Camera");
    setPlayerAuthMode(true);
    authData = browser.getAuthData();
    refreshWidgets();
    sp.browser.setVisible(true);
  });

  sp.once("loadGame", () => {
    // In non-offline mode we still want to see our face in RaceMenu
    const ironHelment = sp.Armor.from(sp.Game.getFormEx(0x00012e4d));
    const pl = sp.Game.getPlayer();
    if (pl) pl.unequipItem(ironHelment, false, true);

    sp.browser.setFocused(true);
    browser.keepCursorMenuOpenedWhenBrowserFocused();
    checkLoginState();
  });

  loadGameManager.loadGame(
    location.pos,
    location.rot,
    location.worldOrCell
  );
}

declare const window: any;
const browsersideWidgetSetter = () => {
  console.log(new Date());
  const loginWidget = {
    type: "form",
    id: 1,
    caption: "authorization",
    elements: [
      {
        type: "button",
        tags: ["BUTTON_STYLE_GITHUB"],
        hint: "get a colored nickname and mention in news",
        click: () => window.skyrimPlatform.sendMessage(events.openGithub),
      },
      {
        type: "button",
        tags: ["BUTTON_STYLE_PATREON", "ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT"],
        hint: "get a colored nickname and other bonuses for patrons",
        click: () => window.skyrimPlatform.sendMessage(events.openPatreon),
      },
      {
        type: "icon",
        text: "username",
        tags: ["ICON_STYLE_SKYMP"],
      },
      {
        type: "text",
        text: (
          authData ? (
            authData.discordUsername
              ? `${authData.discordUsername}`
              : `id: ${authData.masterApiId}`
          ) : "Please log in"
        ),
        tags: ["ELEMENT_SAME_LINE", "ELEMENT_STYLE_MARGIN_EXTENDED"],
      },
      {
        type: "icon",
        text: "discord",
        tags: ["ICON_STYLE_DISCORD"],
      },
      {
        type: "button",
        text: authData ? "change account" : "login or register",
        tags: ["ELEMENT_SAME_LINE"],
        click: () => window.skyrimPlatform.sendMessage(events.openDiscordOauth),
        hint: "You can log in or change account at any time",
      },
      {
        type: "button",
        text: "travel to skyrim",
        tags: ["BUTTON_STYLE_FRAME", "ELEMENT_STYLE_MARGIN_EXTENDED"],
        click: () => window.skyrimPlatform.sendMessage(events.login),
        hint: "Connect to the game server",
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

const refreshWidgets = () => {
  if (browserState.failCount) {
    sp.printConsole(`Auth check fail: ${browserState.comment}`);
  }
  sp.browser.executeJavaScript(new FunctionInfo(browsersideWidgetSetter).getText({ events, browserState, authData }));
};
