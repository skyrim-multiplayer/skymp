import {
  Game,
  Utility,
  once
} from "skyrimPlatform";
import { SkympClient } from "./services/services/skympClient";
import * as browser from "./features/browser";
import { verifyLoadOrder } from './features/loadOrder';

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
import { RagdollService } from "./services/services/ragdollService";
import { DeathService } from "./services/services/deathService";
import { ContainersService } from "./services/services/containersService";
import { NetworkingService } from "./services/services/networkingService";
import { RemoteServer } from "./services/services/remoteServer";
import { SpSnippetService } from "./services/services/spSnippetService";
import { SweetTaffyDynamicPerksService } from "./services/services/sweetTaffyDynamicPerksService";
import { SweetTaffySweetCantDropService } from "./services/services/sweetTaffySweetCantDropService";
import { SweetTaffyStaticPerksService } from "./services/services/sweetTaffyStaticPerksService";
import { DisableSkillAdvanceService } from "./services/services/disableSkillAdvanceService";
import { DisableFastTravelService } from "./services/services/disableFastTravelService";
import { DisableDifficultySelectionService } from "./services/services/disableDifficultySelectionService";
import { SweetTaffyPlayerCombatService } from "./services/services/sweetTaffyPlayerCombatService";
import { WorldCleanerService } from "./services/services/worldCleanerService";
import { SweetTaffySkillMenuService } from "./services/services/sweetTaffySkillMenuService";

browser.main();

once("update", () => {
  Utility.setINIBool("bAlwaysActive:General", true);
  Game.setGameSettingInt("iDeathDropWeaponChance", 0);
});

once("update", verifyLoadOrder);

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
      new RagdollService(sp, controller),
      new DeathService(sp, controller),
      new ContainersService(sp, controller),
      new NetworkingService(sp, controller),
      new RemoteServer(sp, controller),
      new SpSnippetService(sp, controller),
      new SweetTaffyDynamicPerksService(sp, controller),
      new SweetTaffyStaticPerksService(sp, controller),
      new SweetTaffySweetCantDropService(sp, controller),
      new SweetTaffyPlayerCombatService(sp, controller),
      new SweetTaffySkillMenuService(sp, controller),
      new DisableSkillAdvanceService(sp, controller),
      new DisableFastTravelService(sp, controller),
      new DisableDifficultySelectionService(sp, controller),
      new WorldCleanerService(sp, controller)
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
