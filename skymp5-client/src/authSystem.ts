import { Guid } from "guid-typescript";

import * as sp from "skyrimPlatform";
import * as browser from "./browser";
import * as loadGameManager from "./loadGameManager";
import { AuthGameData, LoginRegisterData, LoginResponseAuthData, RemoteAuthGameData } from "./authModel";
import { Transform } from "./movement";
import { escapeJs, nameof } from "./utils";

const authUrl = (sp.settings["skymp5-client"]["master"] as string) || "https://skymp.io";
const githubUrl = "https://github.com/skyrim-multiplayer/skymp";
const patreonUrl = "https://www.patreon.com/skymp";
const loginEventKey = "loginRequiredEvent";
const loginWidgetInfoObjName = "loginWidgetInfo";
const registerEventKey = "registerRequiredEvent";
const registerWidgetInfoObjName = "registerWidgetInfo";
const openGitHubEventKey = "openGithub";
const openPatreonEvetnKey = "openPatreon";
const clearSavedAuthDataEventKey = "clearAuthData";

/**
 * https://github.com/typestack/class-validator/blob/develop/src/validation/ValidationError.ts
 */
interface AuthServerValidationError {
  target?: object;
  property: string;
  value?: any;
  constraints?: {
    [type: string]: string;
  };
  children?: AuthServerValidationError[];
  contexts?: {
    [type: string]: any;
  };
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
    loadLobby(lobbyLocation);
  }
}

function createPlaySession(token: string) {
  const client = new sp.HttpClient(authUrl);
  return client.post(`/api/users/me/play/${sp.settings["skymp5-client"]["server-ip"]}:${sp.settings["skymp5-client"]["server-port"]}`, {
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
    const eventKey = e.arguments[0];
    switch (eventKey) {
      case registerEventKey:
        registerAccountWithSkympIO(e.arguments[1] as LoginRegisterData);
        break;
      case loginEventKey:
        const loginData = e.arguments[1] as LoginRegisterData;
        if (authData && !loginData.changed) {
          onAuthListeners({ remote: { email: loginData.email, rememberMe: loginData.rememberMe ?? false, session: authData.session } });
          return;
        }

        setLoginInfo("processing...");
        loginWithSkympIO(loginData, (msg) => setLoginInfo(msg), (loginResponse) =>
          createPlaySession(loginResponse.token).then((playSession) => {
            onAuthListeners({ remote: { email: loginData.email, rememberMe: loginData.rememberMe ?? false, session: playSession } })
          })
        );
        break;
      case clearSavedAuthDataEventKey:
        browser.setAuthData(null);
        break;
      case openGitHubEventKey:
        sp.win32.loadUrl(githubUrl);
        break;
      case openPatreonEvetnKey:
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

const loadLobby = (location: Transform): void => {
  sp.once("update", () => {
    // sp.Game.setInChargen(true, true, false);
    // sp.Utility.setINIBool("bAlwaysActive:General", true);
    // sp.Utility.setINIFloat("fAutoVanityModeDelay:Camera", 72000.0);
    // sp.Game.enableFastTravel(false);
    //sp.Game.getPlayer()!.setDontMove(true);
    // sp.Game.forceFirstPerson();

    startListenBrowserMessage();
    authData = browser.getAuthData();
    const loginWidgetLoginDataJs = `window.loginData = ${authData ? JSON.stringify(authData) : "{}"};`;
    sp.browser.executeJavaScript(`
    ${loginWidgetLoginDataJs}
    ${loginWidgetJs}
    ${registerWidgetJs}
    window.skyrimPlatform.widgets.set([window.loginWidget]);
    `);
    browser.setBrowserVisible(true);
  });
  // todo: for now, the cursor disappears after a few seconds after the browser receives focus. Need to press F6 twice
  // sp.once("loadGame", () => browser.setBrowserFocused(true));

  loadGameManager.loadGame(
    location.pos,
    location.rot,
    location.worldOrCell
  );
}

const loginWithSkympIO = (data: LoginRegisterData, failCallback: (msg: string) => void, successCallback: (loginResponse: LoginResponseAuthData) => void): void => {
  if (!LoginRegisterData.canLogin(data)) {
    failCallback("Some fields are invalid");
    return;
  }

  new sp.HttpClient(authUrl)
    .post("/api/users/login", { body: JSON.stringify(data), contentType: "application/json" })
    .then(response => {
      switch (response.status) {
        case 200:
          successCallback(JSON.parse(response.body));
          break;
        case 401: // Unauthorized
        case 403: // Forbidden
          failCallback(`${response.body}`);
          break;
        case 404: // Not found
          failCallback(`Login url is invalid (not found)`);
          break;
        default:
          failCallback(`Server returned ${escapeJs(response.status.toString() || "???")} \\"${escapeJs(response.body)}\\"`);
      }
    })
    .catch(reason => {
      if (typeof reason === "string") {
        failCallback(`Skyrim platform error (http): ${reason}`)
      } else {
        failCallback(`Skyrim platform error (http): request rejected`);
      }
    });
}
const setLoginInfo = (text: string): void => {
  sp.browser.executeJavaScript(`
  window.${loginWidgetInfoObjName}.text = "${escapeJs(text)}";
  window.skyrimPlatform.widgets.set([window.loginWidget]);
  `);
}

const registerAccountWithSkympIO = (data: LoginRegisterData): void => {
  if (!LoginRegisterData.canRegister(data)) {
    setRegisterInfo("Registration data is incorrect");
    return;
  };

  setRegisterInfo("processing...");
  data.name = Guid.create().toString().replace(/-/g, "");
  data.rememberMe = true;
  new sp.HttpClient(authUrl)
    .post("/api/users", { body: JSON.stringify(data), contentType: "application/json" })
    .then(response => {
      switch (response.status) {
        case 200:
        case 201:
          loginWithSkympIO(data, (msg) => setRegisterInfo(msg), (loginResponse) =>
            createPlaySession(loginResponse.token).then((playSession) => {
              onAuthListeners({ remote: { email: data.email, rememberMe: true, session: playSession } })
            })
          );
          break;
        case 400: // Bad Request
          if (response.body.startsWith("[")) {
            const errors = JSON.parse(response.body) as AuthServerValidationError[];
            if (errors.length === 0) {
              setRegisterInfo(`Bad Request`);
            } else {
              setRegisterInfo(`${errors[0].property} is invalid`);
            }
          } else {
            setRegisterInfo(`${response.body}`);
          }
          break;
        case 500: // Internal Server Error
          if (response.body) {
            setRegisterInfo(`${response.body}`);
          } else {
            setRegisterInfo(`Internal Server Error`);
          }
          break;
        default:
          setRegisterInfo(`Server returned ${escapeJs(response.status.toString())} \\"${escapeJs(response.body)}\\"`);
          break;
      }
    }).catch(reason => {
      if (typeof reason === "string") {
        setRegisterInfo(`Skyrim platform error (http): ${reason}`)
      } else {
        setRegisterInfo(`Skyrim platform error (http): request rejected`);
      }
    });
}
const setRegisterInfo = (text: string): void => {
  sp.browser.executeJavaScript(`
  window.${registerWidgetInfoObjName}.text = "${escapeJs(text)}";
  window.skyrimPlatform.widgets.set([window.registerWidget]);
  `);
}

const loginWidgetJs = `
window.${loginWidgetInfoObjName} = {
  type: "text",
  text: "",
  tags: [],
};
window.loginWidget = {
  type: "form",
  id: 1,
  caption: "authorization",
  elements: [
    {
      type: "button",
      tags: ["BUTTON_STYLE_GITHUB"],
      hint: "get a colored nickname and mention in news",
      click: () => window.skyrimPlatform.sendMessage("${openGitHubEventKey}")
    },
    {
      type: "button",
      tags: ["BUTTON_STYLE_PATREON", "ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT"],
      hint: "get a colored nickname and other bonuses for patrons",
      click: () => window.skyrimPlatform.sendMessage("${openPatreonEvetnKey}")
    },
    {
      type: "icon",
      text: "email",
      tags: ["ICON_STYLE_MAIL"]
    },
    {
      type: "inputText",
      tags: ["ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT", "ELEMENT_STYLE_MARGIN_EXTENDED"],
      placeholder: "dude33@gmail.com",
      initialValue: window.loginData["${nameof<RemoteAuthGameData>("email")}"] || undefined,
      hint: "enter your e-mail and password for authorization",
      onInput: (e) => {
        window.loginData["${nameof<LoginRegisterData>("email")}"] = e.target.value;
        window.loginData["${nameof<LoginRegisterData>("changed")}"] = true;
      }
    },
    { 
      type: "icon", 
      text: "password", 
      tags: ["ICON_STYLE_KEY"] 
    },
    {
      type: "inputPass",
      tags: ["ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT"],
      placeholder: "password, you know",
      initialValue: window.loginData["${nameof<RemoteAuthGameData>("email")}"] || undefined,
      hint: "enter your e-mail and password for authorization",
      onInput: (e) => {
        window.loginData["${nameof<LoginRegisterData>("password")}"] = e.target.value;
        window.loginData["${nameof<LoginRegisterData>("changed")}"] = true;
      }
    },
    {
      type: "checkBox",
      text: "remember me",
      tags: ["HINT_STYLE_LEFT"],
      hint: "check the box “remember me” for automatic authorization",
      initialValue: window.loginData["${nameof<RemoteAuthGameData>("rememberMe")}"],
      setChecked: (checked) => {
        if (!checked) {
          window.skyrimPlatform.sendMessage("${clearSavedAuthDataEventKey}");
        }
        window.loginData["${nameof<LoginRegisterData>("rememberMe")}"] = checked;
      }
    },
    {
      type: "button",
      text: "register now",
      tags: ["ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT"],
      hint: "click “register now” to create a new account",
      click: () => window.skyrimPlatform.widgets.set([registerWidget])
    },
    window.${loginWidgetInfoObjName},
    {
      type: "button",
      text: "travel to skyrim",
      tags: ["BUTTON_STYLE_FRAME", "ELEMENT_STYLE_MARGIN_EXTENDED"],
      click: () => window.skyrimPlatform.sendMessage("${loginEventKey}", window.loginData)
    }
  ]
};`;

const registerWidgetJs = `
window.${registerWidgetInfoObjName} = {
  type: "text",
  text: "",
  tags: []
};
window.registerWidgetRegisterButton = {
  type: "button",
  tags: ["BUTTON_STYLE_FRAME", "ELEMENT_STYLE_MARGIN_EXTENDED"],
  text: "create account",
  isDisabled: false,
  style: {
    marginTop: "10px"
  },
  click: () => {
    const isPassEqual = window.registerData["${nameof<LoginRegisterData>("password")}"] === window.registerData["${nameof<LoginRegisterData>("passwordRepeat")}"];
    if (!isPassEqual) {
      window.${registerWidgetInfoObjName}.text = "Passwords do not match";
      window.skyrimPlatform.widgets.set([window.registerWidget]);
      return;
    }
    window.skyrimPlatform.sendMessage("${registerEventKey}", window.registerData);
  }
};
window.registerWidgetGoBackButton = {
  type: "button",
  tags: ["BUTTON_STYLE_FRAME", "ELEMENT_STYLE_MARGIN_EXTENDED"],
  text: "go back",
  isDisabled: false,
  style: {
    marginTop: "10px"
  },
  click: () => window.skyrimPlatform.widgets.set([window.loginWidget])
};
window.updateRegisterData = function(propName, value) {
  window.registerData[propName] = value;
  const isRegisterDataHasProps = Object.keys(window.registerData).some(prop => !!window.registerData[prop]);
  const button = isRegisterDataHasProps ? window.registerWidgetRegisterButton : window.registerWidgetGoBackButton;
  window.registerWidget.elements[window.registerWidget.elements.length - 1] = button;
  window.skyrimPlatform.widgets.set([window.registerWidget]);
};
window.registerData = {};
window.registerWidget = {
  type: "form",
  id: 1,
  caption: "Register",
  elements: [
    {
      type: "button",
      tags: ["BUTTON_STYLE_GITHUB"],
      hint: "get a colored nickname and mention in news",
      click: () => window.skyrimPlatform.sendMessage("${openGitHubEventKey}")
    },
    {
      type: "button",
      tags: ["BUTTON_STYLE_PATREON", "ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT"],
      hint: "get a colored nickname and other bonuses for patrons",
      click: () => window.skyrimPlatform.sendMessage("${openPatreonEvetnKey}")
    },
    {
      type: "icon",
      text: "email",
      tags: ["ICON_STYLE_MAIL"]
    },
    {
      type: "inputText",
      tags: ["ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT", "ELEMENT_STYLE_MARGIN_EXTENDED"],
      placeholder: "dude33@gmail.com",
      hint: "enter your e-mail and password for registration",
      onInput: (e) => window.updateRegisterData("${nameof<LoginRegisterData>("email")}", e.target.value)
    },
    {
      type: "icon", 
      text: "password", 
      tags: ["ICON_STYLE_KEY"] 
    },
    {
      type: "inputPass",
      tags: ["ELEMENT_SAME_LINE", "HINT_STYLE_LEFT"],
      placeholder: "password, you know",
      hint: "enter your e-mail and password for authorization",
      onInput: (e) => window.updateRegisterData("${nameof<LoginRegisterData>("password")}", e.target.value)
    },
    { 
      type: "icon", 
      text: "password", 
      tags: ["ICON_STYLE_KEY"] 
    },
    {
      type: "inputPass",
      tags: ["ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT"],
      placeholder: "password, again",
      hint: "confirm your password",
      onInput: (e) => window.updateRegisterData("${nameof<LoginRegisterData>("passwordRepeat")}", e.target.value)
    },
    window.${registerWidgetInfoObjName},
    window.registerWidgetGoBackButton
  ]
};`;
