import {
  Game,
  Utility,
  on,
  once,
  printConsole,
  ActorValueInfo,
  ActorValue,
} from "skyrimPlatform";
import * as timers from "./extensions/timers"; timers;
import { SkympClient } from "./services/services/skympClient";
import * as browser from "./features/browser";
import { updateWc } from "./features/worldCleaner";
import { verifyLoadOrder } from './features/loadOrder';
import * as expSystem from "./sync/expSystem";
import * as skillSystem from "./features/skillMenu";

import * as sp from "skyrimPlatform";

import { BlockPapyrusEventsService } from './services/services/blockPapyrusEventsService';
import { EnforceLimitationsService } from './services/services/enforceLimitationsService';
import { LoadGameService } from './services/services/loadGameService';
import { SendInputsService } from './services/services/sendInputsService';
import { SinglePlayerService } from './services/services/singlePlayerService';
import { SpApiInteractor } from './services/spApiInteractor';
import { TimeService } from "./services/services/timeService";
import { SpVersionCheckService } from "./services/services/spVersionCheckService";

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

skillSystem.skillMenuInit();

const main = () => {
  try {
    const controller = SpApiInteractor.makeController();
    
    SpApiInteractor.setup([
      new BlockPapyrusEventsService(sp, controller),
      new LoadGameService(sp, controller),
      new SinglePlayerService(sp, controller),
      new EnforceLimitationsService(sp, controller),
      new SendInputsService(sp, controller),
      new SkympClient(sp, controller),
      new TimeService(sp, controller),
      new SpVersionCheckService(sp, controller)
    ]);
  }
  catch (e) {
    // TODO: handle setup failure. will output to game console by default
    throw e;
  }
};
main();
