import { Transform } from './sync/movement';
import {
  Game,
  Utility,
  on,
  once,
  GlobalVariable,
  ObjectReference,
  settings,
  storage,
  browser as spBrowser,
  printConsole
} from "skyrimPlatform";
import { connectWhenICallAndNotWhenIImport, SkympClient } from "./skympClient";
import * as browser from "./features/browser";
import * as loadGameManager from "./features/loadGameManager";
import { verifyVersion } from "./version";
import { updateWc } from "./features/worldCleaner";
import * as authSystem from "./features/authSystem";
import { AuthGameData } from "./features/authModel";
import * as NetInfo from "./features/netInfoSystem";
import * as playerCombatSystem from "./sweetpie/playerCombatSystem";
import { verifyLoadOrder } from './features/loadOrder';

browser.main();

export const defaultLocalDamageMult = 1;
export const setLocalDamageMult = (damageMult: number): void => {
  Game.setGameSettingFloat("fDiffMultHPToPCE", damageMult);
  Game.setGameSettingFloat("fDiffMultHPToPCH", damageMult);
  Game.setGameSettingFloat("fDiffMultHPToPCL", damageMult);
  Game.setGameSettingFloat("fDiffMultHPToPCN", damageMult);
  Game.setGameSettingFloat("fDiffMultHPToPCVE", damageMult);
  Game.setGameSettingFloat("fDiffMultHPToPCVH", damageMult);
}

const enforceLimitations = () => {
  Game.setInChargen(true, true, false);
};

once("update", enforceLimitations);
loadGameManager.addLoadGameListener(enforceLimitations);

once("update", () => {
  Utility.setINIBool("bAlwaysActive:General", true);
  Game.setGameSettingInt("iDeathDropWeaponChance", 0);
  setLocalDamageMult(defaultLocalDamageMult);
});
on("update", () => {
  Utility.setINIInt("iDifficulty:GamePlay", 5);
  Game.enableFastTravel(false);
});

on("update", () => updateWc());

once("update", verifyLoadOrder);

const startClient = (): void => {
  NetInfo.start();

  playerCombatSystem.start();
  once("update", () => authSystem.setPlayerAuthMode(false));
  connectWhenICallAndNotWhenIImport();
  new SkympClient();

  once("update", verifyVersion);

  let lastTimeUpd = 0;
  on("update", () => {
    if (Date.now() - lastTimeUpd <= 2000) return;
    lastTimeUpd = Date.now();

    const gameHourId = 0x38;
    const gameMonthId = 0x36;
    const gameDayId = 0x37;
    const gameYearId = 0x35;
    const timeScaleId = 0x3a;

    const gameHour = GlobalVariable.from(Game.getFormEx(gameHourId));
    const gameDay = GlobalVariable.from(Game.getFormEx(gameDayId));
    const gameMonth = GlobalVariable.from(Game.getFormEx(gameMonthId));
    const gameYear = GlobalVariable.from(Game.getFormEx(gameYearId));
    const timeScale = GlobalVariable.from(Game.getFormEx(timeScaleId));
    if (!gameHour || !gameDay || !gameMonth || !gameYear || !timeScale) {
      return;
    }

    const d = new Date();

    let newGameHourValue = 0;
    newGameHourValue += d.getUTCHours();
    newGameHourValue += d.getUTCMinutes() / 60;
    newGameHourValue += d.getUTCSeconds() / 60 / 60;
    newGameHourValue += d.getUTCMilliseconds() / 60 / 60 / 1000;

    const diff = Math.abs(gameHour.getValue() - newGameHourValue);

    if (diff >= 1 / 60) {
      gameHour.setValue(newGameHourValue);
      gameDay.setValue(d.getUTCDate());
      gameMonth.setValue(d.getUTCMonth());
      gameYear.setValue(d.getUTCFullYear() - 2020 + 199);
    }

    timeScale.setValue(gameHour.getValue() > newGameHourValue ? 0.6 : 1.2);
  });

  let riftenUnlocked = false;
  on("update", () => {
    if (riftenUnlocked) return;
    const refr = ObjectReference.from(Game.getFormEx(0x42284));
    if (!refr) return;
    refr.lock(false, false);
    riftenUnlocked = true;
  });

  const n = 10;
  let k = 0;
  let zeroKMoment = 0;
  let lastFps = 0;
  on("update", () => {
    ++k;
    if (k == n) {
      k = 0;
      if (zeroKMoment) {
        const timePassed = (Date.now() - zeroKMoment) * 0.001;
        const fps = Math.round(n / timePassed);
        if (lastFps != fps) {
          lastFps = fps;
          //printConsole(`Current FPS is ${fps}`);
        }
      }
      zeroKMoment = Date.now();
    }
  });
}

const authGameData = storage[AuthGameData.storageKey] as AuthGameData | undefined;
if (!(authGameData?.local || authGameData?.remote)) {
  authSystem.addAuthListener((data) => {
    if (data.remote) {
      browser.setAuthData(data.remote);
    }
    storage[AuthGameData.storageKey] = data;
    spBrowser.setFocused(false);
    startClient();
  });

  authSystem.main(settings["skymp5-client"]["lobbyLocation"] as Transform);
} else {
  startClient();
}
