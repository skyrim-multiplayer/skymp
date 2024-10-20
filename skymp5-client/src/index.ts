import {
  Game,
  Utility,
  once
} from "skyrimPlatform";
import { SkympClient } from "./services/services/skympClient";

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
import { LoadOrderVerificationService } from "./services/services/loadOrderVerificationService";
import { BrowserService } from "./services/services/browserService";
import { AuthService } from "./services/services/authService";
import { NetInfoService } from "./services/services/netInfoService";
import { AnimDebugService } from "./services/services/animDebugService";
import { TimersService } from "./services/services/timersService";
import { PlayerBowShotService } from "./services/services/playerBowShotService";
import { GamemodeEventSourceService } from "./services/services/gamemodeEventSourceService";
import { GamemodeUpdateService } from "./services/services/gamemodeUpdateService";
import { FrontHotReloadService } from "./services/services/frontHotReloadService";
import { BlockedAnimationsService } from "./services/services/blockedAnimationsService";
import { WorldView } from "./view/worldView";
import { KeyboardEventsService } from "./services/services/keyboardEventsService";
import { MagicSyncService } from "./services/services/magicSyncService";

once("update", () => {
  Utility.setINIBool("bAlwaysActive:General", true);
  Game.setGameSettingInt("iDeathDropWeaponChance", 0);
});

const main = () => {
  try {
    const controller = SpApiInteractor.getControllerInstance();

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
      new WorldCleanerService(sp, controller),
      new LoadOrderVerificationService(sp, controller),
      new BrowserService(sp, controller),
      new AuthService(sp, controller),
      new NetInfoService(sp, controller),
      new AnimDebugService(sp, controller),
      new TimersService(sp, controller),
      new PlayerBowShotService(sp, controller),
      new GamemodeEventSourceService(sp, controller),
      new GamemodeUpdateService(sp, controller),
      new FrontHotReloadService(sp, controller),
      new BlockedAnimationsService(sp, controller),
      new WorldView(sp, controller),
      new KeyboardEventsService(sp, controller),
      new MagicSyncService(sp, controller)
    ];
    SpApiInteractor.setup(listeners);
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
