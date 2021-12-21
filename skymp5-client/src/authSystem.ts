import { Guid } from "guid-typescript";

import * as sp from "skyrimPlatform";
import * as spe from "./skyrimPlatform.extensions";
import * as browser from "./browser";
import * as loadGameManager from "./loadGameManager";
import { AuthData, TokenAuthData } from "./authData";
import { Transform } from "./movement";
import { escapeJs, nameof } from "./utils";

const authUrl = "https://skymp.io";
const githubUrl = "https://github.com/skyrim-multiplayer/skymp";
const patreonUrl = "https://www.patreon.com/skymp";
const loginEventKey = "loginRequiredEvent";
const loginWidgetInfoObjName = "loginWidgetInfo";
const registerEventKey = "registerRequiredEvent";
const registerWidgetInfoObjName = "registerWidgetInfo";
const openGHEventKey = "openGithub";
const openPatreonEvetnKey = "openPatreon";

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

export type AuthCallback = (data: TokenAuthData) => void;

const authListeners = new Array<AuthCallback>();
export const addAuthListener = (callback: AuthCallback): void => {
  authListeners.push(callback);
}

export const main = (lobbyLocation: Transform): void => {
  const authData = browser.getAuthData();
  if (authData) {
    authListeners.forEach(listener => listener(authData));
  } else {
    loadLobby(lobbyLocation);
  }
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
        registerAccountWithSkympIO(e.arguments[1] as AuthData);
        break;
      case loginEventKey:
        loginWithSkympIO(e.arguments[1] as AuthData);
        break;
      case openGHEventKey:
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
    sp.Game.setInChargen(true, true, false);
    sp.Utility.setINIBool("bAlwaysActive:General", true);
    sp.Utility.setINIBool("bDisableAutoVanityMode:Camera", true);
    sp.Utility.setINIFloat("fAutoVanityModeDelay:Camera", 3600.0);
    sp.Game.enableFastTravel(false);
    sp.Game.getPlayer()!.setDontMove(true);
    sp.Game.forceFirstPerson();

    startListenBrowserMessage();
    sp.browser.executeJavaScript(`
    ${loginWidgetJs}
    ${registerWidgetJs}
    window.skyrimPlatform.widgets.set([window.loginWidget]);
    `);
    browser.setBrowserVisible(true);
  });
  sp.once("loadGame", () => browser.setBrowserFocused(true));

  loadGameManager.loadGame(
    location.pos,
    location.rot,
    location.worldOrCell
  );
}

const loginWithSkympIO = (data: AuthData): void => {
  if (!AuthData.canLogin(data)) {
    setLoginInfo("Some fields are invalid");
    return;
  }

  setLoginInfo("processing...");
  new sp.HttpClient(authUrl)
    .post("/api/users/login", { body: JSON.stringify(data), contentType: "application/json" })
    .then(response => {
      switch (response.status) {
        case 200:
          setLoginInfo("SUCCESS");
          break;
        case 401: // Unauthorized
        case 403: // Forbidden
          setLoginInfo(`${response.body}`);
          break;
        case 404: // Not found
          setLoginInfo(`Login url is invalid (not found)`);
          break;
        default:
          setLoginInfo(`Server returned ${response.status} "${response.body}"`);
      }
    })
    .catch(reason => {
      if (typeof reason === "string") {
        setLoginInfo(`Skyrim platform error (http): ${reason}`)
      } else {
        setLoginInfo(`Skyrim platform error (http): request rejected`);
      }
    });
}
const setLoginInfo = (text: string): void => {
  sp.browser.executeJavaScript(`
  window.${loginWidgetInfoObjName}.text = "${escapeJs(text)}";
  window.skyrimPlatform.widgets.set([window.loginWidget]);
  `);
}

const registerAccountWithSkympIO = (data: AuthData): void => {
  if (!AuthData.canRegister(data)) {
    setRegisterInfo("Registration data is incorrect");
    return;
  };

  setRegisterInfo("processing...");
  data.name = Guid.create().toString().replace(/-/g, "");
  new sp.HttpClient(authUrl)
    .post("/api/users", { body: JSON.stringify(data), contentType: "application/json" })
    .then(response => {
      switch (response.status) {
        case 200:
        case 201:
          setRegisterInfo("SUCCESS");
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
          setRegisterInfo(`Server returned ${response.status} "${response.body}"`);
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
window.loginData = {};
window.loginWidget = {
  type: "form",
  id: 1,
  caption: "authorization",
  elements: [
    {
      type: "button",
      tags: ["BUTTON_STYLE_GITHUB"],
      hint: "get a colored nickname and mention in news",
      click: () => window.skyrimPlatform.sendMessage("${openGHEventKey}")
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
      hint: "enter your e-mail and password for authorization",
      onInput: (e) => window.loginData["${nameof<AuthData>("email")}"] = e.target.value
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
      hint: "enter your e-mail and password for authorization",
      onInput: (e) => window.loginData["${nameof<AuthData>("password")}"] = e.target.value
    },
    {
      type: "checkBox",
      text: "remember me",
      tags: ["HINT_STYLE_LEFT"],
      hint: "check the box “remember me” for automatic authorization",
      onInput: (e) => window.loginData["${nameof<AuthData>("rememberMe")}"] = e.target.value
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
    const isPassEqual = window.registerData["${nameof<AuthData>("password")}"] === window.registerData["${nameof<AuthData>("passwordRepeat")}"];
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
      type: "icon",
      text: "email",
      tags: ["ICON_STYLE_MAIL"]
    },
    {
      type: "inputText",
      tags: ["ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT", "ELEMENT_STYLE_MARGIN_EXTENDED"],
      placeholder: "dude33@gmail.com",
      hint: "enter your e-mail and password for registration",
      onInput: (e) => window.updateRegisterData("${nameof<AuthData>("email")}", e.target.value)
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
      onInput: (e) => window.updateRegisterData("${nameof<AuthData>("password")}", e.target.value)
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
      onInput: (e) => window.updateRegisterData("${nameof<AuthData>("passwordRepeat")}", e.target.value)
    },
    window.${registerWidgetInfoObjName},
    window.registerWidgetGoBackButton
  ]
};`;
