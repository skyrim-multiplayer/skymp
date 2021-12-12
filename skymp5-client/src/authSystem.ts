import { AuthData, TokenAuthData } from './authData';
import * as sp from "skyrimPlatform";
import * as browser from "./browser";
import * as loadGameManager from "./loadGameManager";
import { Transform } from "./movement";
import { nameof } from "./utils";

const loginEventKey = "loginRequiredEvent";
const registerEventKey = "registerRequiredEvent";

export type AuthCallback = (data: TokenAuthData) => void;

const authListeners = new Array<AuthCallback>();
export const addAuthListener = (callback: AuthCallback): void => {
  authListeners.push(callback);
}

export const main = (location: Transform): void => {
  const authData = browser.getAuthData();
  if (authData) {
    authListeners.forEach(listener => listener(authData));
  } else {
    loadLobby(location);
  }
};

const loadLobby = (location: Transform): void => {
  sp.once("update", () => {
    sp.Game.setInChargen(true, true, false);
    sp.Game.enableFastTravel(false);
    //sp.Game.getPlayer()!.setDontMove(true);
    sp.Game.forceFirstPerson();

    sp.browser.executeJavaScript(loginWidgetJs);
    sp.browser.executeJavaScript(registerWidgetJs);
    sp.browser.executeJavaScript(`window.skyrimPlatform.widgets.set([window.loginWidget]);`);
    browser.setBrowserVisible(true);
    //browser.setBrowserFocused(true);
  }); 

  loadGameManager.loadGame(
    location.pos,
    location.rot,
    location.worldOrCell
  );
}

const loginWidgetJs = `window.loginWidget = {
  type: "form",
  id: 1,
  caption: "Login",
  elements: [
    { type: "button", tags: ["BUTTON_STYLE_GITHUB"] },
    { type: "button", tags: ["BUTTON_STYLE_PATREON", "ELEMENT_SAME_LINE"] },
    { type: "icon", text: "email", tags: ["ICON_STYLE_MAIL"] },
    { type: "inputText", tags: ["ELEMENT_SAME_LINE"], placeholder: "dude33@gmail.com" },
    { type: "icon", text: "password", tags: ["ICON_STYLE_KEY"] },
    { type: "inputPass", tags: ["ELEMENT_SAME_LINE"], placeholder: "password, you know" },
    { type: "checkBox", text: "remember me" },
    { type: "button", text: "register now", tags: ["ELEMENT_SAME_LINE", "BUTTON_STYLE_FRAME"] },
    {
      type: "button",
      text: "travel to skyrim",
      tags: ["BUTTON_STYLE_FRAME"],
      click: () =>
        window.skyrimPlatform.sendMessage(
          "${loginEventKey}",
          {
            ${nameof<AuthData>("name")}: "", 
            ${nameof<AuthData>("email")}: window.loginWidget.elemets[3].text,
            ${nameof<AuthData>("password")}: window.loginWidget.elemets[5].text,
            ${nameof<AuthData>("rememberMe")}: window.loginWidget.elements[6].value || false,
          }
        )
    },
  ]
};`;

const registerWidgetJs = `window.registerWidget = {
  type: "form",
  id: 2,
  caption: "Create account",
  elemets: [
    { type: "button", tags: ["BUTTON_STYLE_GITHUB"] },
    { type: "button", tags: ["BUTTON_STYLE_PATREON", "ELEMENT_SAME_LINE"] },
    { type: "icon", text: "email", tags: ["ICON_STYLE_MAIL"] },
    { type: "inputText", tags: ["ELEMENT_SAME_LINE"], placeholder: "dude33@gmail.com" },
    { type: "icon", text: "character name", tags: [] },
    { type: "inputText", tags: ["ELEMENT_SAME_LINE"], placeholder: "Dovahkiin100500" },
    { type: "icon", text: "password", tags: ["ICON_STYLE_KEY"] },
    { type: "inputPass", tags: ["ELEMENT_SAME_LINE"], placeholder: "password, you know" },
    { type: "checkBox", text: "remember me" },
    {
      type: "button",
      text: "register",
      tags: ["BUTTON_STYLE_FRAME"],
      click: () =>
        window.skyrimPlatform.sendMessage(
          "${registerEventKey}",
          {
            ${nameof<AuthData>("name")}: window.loginWidget.elemets[5].text, 
            ${nameof<AuthData>("email")}: window.loginWidget.elemets[3].text,
            ${nameof<AuthData>("password")}: window.loginWidget.elemets[7].text,
            ${nameof<AuthData>("rememberMe")}: window.loginWidget.elements[8].value ?? false,
          }
        )
    },
  ]
};`;
