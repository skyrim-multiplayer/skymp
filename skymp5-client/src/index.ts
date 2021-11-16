import {
  Game,
  Utility,
  on,
  once,
  GlobalVariable,
  ObjectReference,
  Weather,
} from "skyrimPlatform";
import { SkympClient } from "./skympClient";
import * as browser from "./browser";
import * as loadGameManager from "./loadGameManager";
import { verifyVersion } from "./version";
import { updateWc } from "./worldCleaner";

new SkympClient();

const enforceLimitations = () => {
  Game.setInChargen(true, true, false);
};

export const defaultLocalDamageMult = 1;
export const setLocalDamageMult = (damageMult: number): void => {
  Game.setGameSettingFloat("fDiffMultHPToPCE", damageMult);
  Game.setGameSettingFloat("fDiffMultHPToPCH", damageMult);
  Game.setGameSettingFloat("fDiffMultHPToPCL", damageMult);
  Game.setGameSettingFloat("fDiffMultHPToPCN", damageMult);
  Game.setGameSettingFloat("fDiffMultHPToPCVE", damageMult);
  Game.setGameSettingFloat("fDiffMultHPToPCVH", damageMult);
}

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

browser.main();

once("update", verifyVersion);

on("update", () => updateWc());

let lastTimeUpd = 0;
on("update", () => {
  if (Date.now() - lastTimeUpd <= 2000) return;
  lastTimeUpd = Date.now();

  // Also update weather to be always clear
  const w = Weather.findWeather(0);
  if (w) {
    w.setActive(false, false);
  }

  const gameHourId = 0x38;
  const gameMonthId = 0x36;
  const gameDayId = 0x37;
  const gameYearId = 0x35;
  const timeScaleId = 0x3a;

  const d = new Date();

  const gameHour = GlobalVariable.from(Game.getFormEx(gameHourId)) as GlobalVariable;
  gameHour.setValue(
    d.getUTCHours() +
    d.getUTCMinutes() / 60 +
    d.getUTCSeconds() / 60 / 60 +
    d.getUTCMilliseconds() / 60 / 60 / 1000
  );

  const gameDay = GlobalVariable.from(Game.getFormEx(gameDayId)) as GlobalVariable;
  gameDay.setValue(d.getUTCDate());

  const gameMonth = GlobalVariable.from(Game.getFormEx(gameMonthId)) as GlobalVariable;
  gameMonth.setValue(d.getUTCMonth());

  const gameYear = GlobalVariable.from(Game.getFormEx(gameYearId)) as GlobalVariable;
  gameYear.setValue(d.getUTCFullYear() - 2020 + 199);

  const timeScale = GlobalVariable.from(Game.getFormEx(timeScaleId)) as GlobalVariable;
  timeScale.setValue(1);
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
