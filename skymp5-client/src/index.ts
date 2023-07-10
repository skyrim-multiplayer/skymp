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
  printConsole,
  ActorValueInfo,
  ActorValue,
} from "skyrimPlatform";
import * as timers from "./extensions/timers"; timers;
import { connectWhenICallAndNotWhenIImport, SkympClient } from "./skympClient";
import * as browser from "./features/browser";
import * as loadGameManager from "./features/loadGameManager";
import { verifyVersion } from "./version";
import { updateWc } from "./features/worldCleaner";
import * as authSystem from "./features/authSystem";
import { AuthGameData } from "./features/authModel";
import * as NetInfo from "./debug/netInfoSystem";
import * as animDebugSystem from "./debug/animDebugSystem";
import * as playerCombatSystem from "./sweetpie/playerCombatSystem";
import { verifyLoadOrder } from './features/loadOrder';
import * as expSystem from "./sync/expSystem";
import * as skillSystem from "./features/skillMenu";

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

const turnOffSkillLocalExp = (av: ActorValue): void => {
  const avi = ActorValueInfo.getActorValueInfoByID(av);
  if (!avi) {
    once("update", () => printConsole(`Not found "${ActorValueInfo}" with value "${av}"`));
    return;
  }
  avi.setSkillUseMult(0);
  avi.setSkillOffsetMult(0);
};

const enforceLimitations = () => {
  Game.setInChargen(true, true, false);
};

once("update", enforceLimitations);
loadGameManager.addLoadGameListener(enforceLimitations);

once("update", () => {
  Utility.setINIBool("bAlwaysActive:General", true);
  Game.setGameSettingInt("iDeathDropWeaponChance", 0);

  // turn off player level exp
  Game.setGameSettingFloat("fXPPerSkillRank", 0);
  // turn off skill exp
  turnOffSkillLocalExp(ActorValue.Alteration);
  turnOffSkillLocalExp(ActorValue.Conjuration);
  turnOffSkillLocalExp(ActorValue.Destruction);
  turnOffSkillLocalExp(ActorValue.Illusion);
  turnOffSkillLocalExp(ActorValue.Restoration);
  turnOffSkillLocalExp(ActorValue.Enchanting);
  turnOffSkillLocalExp(ActorValue.OneHanded);
  turnOffSkillLocalExp(ActorValue.TwoHanded);
  turnOffSkillLocalExp(ActorValue.Archery);
  turnOffSkillLocalExp(ActorValue.Block);
  turnOffSkillLocalExp(ActorValue.Smithing);
  turnOffSkillLocalExp(ActorValue.HeavyArmor);
  turnOffSkillLocalExp(ActorValue.LightArmor);
  turnOffSkillLocalExp(ActorValue.Pickpocket);
  turnOffSkillLocalExp(ActorValue.Lockpicking);
  turnOffSkillLocalExp(ActorValue.Sneak);
  turnOffSkillLocalExp(ActorValue.Alchemy);
  turnOffSkillLocalExp(ActorValue.Speech);

  // Init exp system
  expSystem.init();

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
  animDebugSystem.init(settings["skymp5-client"]["animDebug"] as animDebugSystem.AnimDebugSettings);

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

skillSystem.skillMenuInit();
