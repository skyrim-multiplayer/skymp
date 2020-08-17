import { SkympClient } from "./skympClient";
import { blockConsole } from "./console";
import * as browser from "./browser";
import * as loadGameManager from "./loadGameManager";
import {
  Game,
  Utility,
  on,
  once,
  GlobalVariable,
  printConsole,
  ObjectReference,
  Weather,
} from "skyrimPlatform";
import { verifyVersion } from "./version";
import { updateWc } from "./worldCleaner";

new SkympClient();

const enforceLimitations = () => {
  Game.setInChargen(true, true, false);
  Game.enableFastTravel(false);
};

once("update", enforceLimitations);
loadGameManager.addLoadGameListener(enforceLimitations);

once("update", () => {
  Utility.setINIBool("bAlwaysActive:General", true);
});
on("update", () => {
  Utility.setINIInt("iDifficulty:GamePlay", 5);
});

browser.main();
blockConsole();

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

  const gameHour = GlobalVariable.from(Game.getFormEx(gameHourId));
  gameHour.setValue(
    d.getUTCHours() +
      d.getUTCMinutes() / 60 +
      d.getUTCSeconds() / 60 / 60 +
      d.getUTCMilliseconds() / 60 / 60 / 1000
  );

  const gameDay = GlobalVariable.from(Game.getFormEx(gameDayId));
  gameDay.setValue(d.getUTCDate());

  const gameMonth = GlobalVariable.from(Game.getFormEx(gameMonthId));
  gameMonth.setValue(d.getUTCMonth());

  const gameYear = GlobalVariable.from(Game.getFormEx(gameYearId));
  gameYear.setValue(d.getUTCFullYear() - 2020 + 199);

  const timeScale = GlobalVariable.from(Game.getFormEx(timeScaleId));
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

/*let lastCrosshairRefId = 0;
on("update", () => {
  const ref = Game.getCurrentCrosshairRef();
  const refId = ref ? ref.getFormID() : 0;
  if (refId === lastCrosshairRefId) return;

  lastCrosshairRefId = refId;
  printConsole("crosshair ref changed to " + refId.toString(16));

  if (!ref) return;

  const base = ref.getBaseObject();
  if (!base) return;

  const processedIds = new Set<number>();
  processedIds.add(refId);

  //dealWithRef(ref, base);

  for (let i = 0; i < 10; ++i) {
    const foundRef = Game.findRandomReferenceOfType(
      base,
      ref.getPositionX(),
      ref.getPositionY(),
      ref.getPositionZ(),
      10000
    );
    const foundRefId = foundRef ? foundRef.getFormID() : 0;
    if (foundRef && !processedIds.has(foundRefId)) {
      //dealWithRef(foundRef, base);
      processedIds.add(foundRefId);
    }
  }
});
*/

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
        printConsole(`Current FPS is ${fps}`);
      }
    }
    zeroKMoment = Date.now();
  }
});
