import { AuthData, TokenAuthData } from "./authData";
import * as sp from "skyrimPlatform";
import * as browser from "./browser";
import * as loadGameManager from "./loadGameManager";
import { Transform } from "./movement";
import { nameof } from "./utils";

const url = "https://skymp.io";
const loginEventKey = "loginRequiredEvent";
const loginWidgetInfoObjName = "loginWidgetInfo";
const registerEventKey = "registerRequiredEvent";
const registerWidgetInfoObjName = "registerWidgetInfo";

export type AuthCallback = (data: TokenAuthData) => void;

const authListeners = new Array<AuthCallback>();
export const addAuthListener = (callback: AuthCallback): void => {
  authListeners.push(callback);
};

export const main = (lobbyLocation: Transform): void => {
  const authData = browser.getAuthData();
  if (authData) {
    authListeners.forEach(listener => listener(authData));
  } else {
    loadLobby(lobbyLocation);
  }
};

sp.on("browserMessage", (msg) => {
  sp.printConsole(msg);
  if (msg.arguments[0] === registerEventKey) {
    registerAccountWithSkympIO(msg.arguments[1] as AuthData);
  } else if (msg.arguments[0] == loginEventKey) {
    loginWithSkympIO(msg.arguments[1] as AuthData);
  }
});

const loadLobby = (location: Transform): void => {
  sp.once("update", () => {
    sp.Game.setInChargen(true, true, false);
    sp.Utility.setINIBool("bAlwaysActive:General", true);
    sp.Game.enableFastTravel(false);
    sp.Game.getPlayer()!.setDontMove(true);
    sp.Game.forceFirstPerson();

    sp.browser.executeJavaScript(`
    ${loginWidgetJs}
    ${registerWidgetJs}
    window.skyrimPlatform.widgets.set([window.loginWidget]);
    `);
    browser.setBrowserVisible(true);
    browser.setBrowserFocused(true);
  });

  loadGameManager.loadGame(
    location.pos,
    location.rot,
    location.worldOrCell
  );
};

const loginWithSkympIO = (data: AuthData): void => {
  if (!AuthData.canLogin(data)) {
    setLoginInfo("Some fields are invalid");
    return;
  }

  setLoginInfo("processing...");
  const http = new sp.HttpClient(url);
  http.post("/api/users/login", { body: JSON.stringify(data), contentType: "application/json" }).then(
    (response) => {
      switch (response.status) {
        case 200:
          setLoginInfo("SUCCESS");
          break;
        case 401:
          setLoginInfo(`Server returned 401 (Unauthorized) "${JSON.stringify(response.body)}"`);
          break;
        case 403:
          setLoginInfo(`Server returned 403 (Forbidden) "${JSON.stringify(response.body)}"`);
          break;
        default:
          setLoginInfo(`Server returned ${response.status} "${JSON.stringify(response.body)}"`);
      }
    },
    (reason) => setLoginInfo(`Skyrim platform error: ${reason}`),
  )
}
const setLoginInfo = (text: string): void => {
  sp.browser.executeJavaScript(`
  window.${loginWidgetInfoObjName}.text = "${text}";
  window.skyrimPlatform.widgets.set([window.loginWidget]);
  `);
}

const registerAccountWithSkympIO = (data: AuthData): void => {
  if (!AuthData.canRegister(data)) {
    setRegisterInfo("Registration data is incorrect");
    return;
  };

  setRegisterInfo("processing...");
  new sp.HttpClient(url)
    .post("/users", { body: JSON.stringify(data), contentType: "application/json" }).then(
      (response) => {
        switch (response.status) {
          case 200:
          case 201:
            setRegisterInfo(`SUCCESS`);
            break;
          case 400:
            setRegisterInfo(`Server returned 400 (Bad Request) "${JSON.stringify(response.body)}"`);
            break;
          case 500:
            setRegisterInfo(`Server returned 500 (Internal Server Error) "${JSON.stringify(response.body)}"`);
            break;
          default:
            setRegisterInfo(`Server returned ${response.status} "${JSON.stringify(response.body)}"`);
            break;
        }
      },
      (reason) => setRegisterInfo(`Skyrim platform error: ${reason}`),
    );
};
const setRegisterInfo = (text: string): void => {
  sp.browser.executeJavaScript(`
  window.${registerWidgetInfoObjName}.text = "${text}";
  window.skyrimPlatform.widgets.set([window.registerWidget]);
  `);
};

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
      hint: "get a colored nickname and mention in news"
    },
    {
      type: "button",
      tags: ["BUTTON_STYLE_PATREON", "ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT"],
      hint: "get a colored nickname and other bonuses for patrons"
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
window.registerData = {};
window.registerWidget = {
  type: "form",
  id: 1,
  caption: "Register",
  elements: [
    {
      type: "button",
      tags: ["BUTTON_STYLE_GITHUB"],
      hint: "get a colored nickname and mention in news"
    },
    {
      type: "button",
      tags: ["BUTTON_STYLE_PATREON", "ELEMENT_SAME_LINE", "HINT_STYLE_RIGHT"],
      hint: "get a colored nickname and other bonuses for patrons"
    },
    { 
      type: "icon", 
      text: "name", 
      tags: []
    },
    { 
      type: "inputText", 
      tags: ["ELEMENT_SAME_LINE"], 
      text: "Dovahkiin3228",
      placeholder: "Dovahkiin3228",
      hint: "character name",
      onInput: (e) => window.registerData["${nameof<AuthData>("name")}"] = e.target.value
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
      onInput: (e) => window.registerData["${nameof<AuthData>("email")}"] = e.target.value
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
      onInput: (e) => window.registerData["${nameof<AuthData>("password")}"] = e.target.value
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
      hint: "confirm your password"
    },
    window.${registerWidgetInfoObjName},
    {
      type: "button",
      tags: ["BUTTON_STYLE_FRAME", "ELEMENT_STYLE_MARGIN_EXTENDED"],
      text: "create account",
      style: {
        marginTop: "10px"
      },
      click: () => window.skyrimPlatform.sendMessage("${registerEventKey}", window.registerData)
    }
  ]
};`;
