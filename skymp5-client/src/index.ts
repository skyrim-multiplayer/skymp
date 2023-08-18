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
import { ConsoleCommandsService } from "./services/services/consoleCommandsService";
import { LastInvService } from "./services/services/lastInvService";
import { ActivationService } from "./services/services/activationService";
import { CraftService } from "./services/services/craftService";
import { DropItemService } from "./services/services/dropItemService";
import { HitService } from "./services/services/hitService";
import { SendMessagesService } from "./services/services/sendMessagesService";
import { RagdollService } from "./services/services/ragdollService";

browser.main();

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
    
    const listeners = [
      new BlockPapyrusEventsService(sp, controller),
      new LoadGameService(sp, controller),
      new SinglePlayerService(sp, controller),
      new EnforceLimitationsService(sp, controller),
      new SendInputsService(sp, controller),
      new SkympClient(sp, controller),
      new TimeService(sp, controller),
      new SpVersionCheckService(sp, controller),
      new ConsoleCommandsService(sp, controller),
      new LastInvService(sp, controller),
      new ActivationService(sp, controller),
      new CraftService(sp, controller),
      new DropItemService(sp, controller),
      new HitService(sp, controller),
      new SendMessagesService(sp, controller),
      new RagdollService(sp, controller)
    ];
    SpApiInteractor.setup(listeners);
    listeners.forEach(listener => SpApiInteractor.registerListenerForLookup(listener.constructor.name, listener));
  }
  catch (e) {
    // TODO: handle setup failure. will output to game console by default
    throw e;
  }
};

// [18.08.2023]
// I saw "attempt to call hooks.add while in hook context" error
// I'm not sure if it's a C++ bug in SkyrimPlatform or an artifact of webpack+hotreload
// But let's for now ensure that "main" executes inside tick context
once("tick", main);
