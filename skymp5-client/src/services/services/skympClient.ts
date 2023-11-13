import {
  on,
  once,
  printConsole,
  settings,
  storage,
} from 'skyrimPlatform';
import * as netInfo from '../../debug/netInfoSystem';
import * as updateOwner from '../../gamemodeApi/updateOwner';
import * as networking from './networkingService';
import { RemoteServer } from './remoteServer';
import { setupHooks } from '../../sync/animation';
import * as animDebugSystem from '../../debug/animDebugSystem';
import { WorldView } from '../../view/worldView';
import { SinglePlayerService } from './singlePlayerService';
import * as authSystem from "../../features/authSystem";
import * as playerCombatSystem from "../../sweetpie/playerCombatSystem";
import { AuthGameData } from '../../features/authModel';
import * as browser from "../../features/browser";
import { ClientListener, CombinedController, Sp } from './clientListener';
import { ConnectionFailed } from '../events/connectionFailed';
import { ConnectionDenied } from '../events/connectionDenied';

printConsole('Hello Multiplayer!');
printConsole('settings:', settings['skymp5-client']);

const targetIp = settings['skymp5-client']['server-ip'] as string;
const targetPort = settings['skymp5-client']['server-port'] as number;

export const getServerIp = () => {
  return targetIp;
};

export const getServerUiPort = () => {
  return targetPort === 7777 ? 3000 : (targetPort as number) + 1;
};

export class SkympClient extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    this.controller.emitter.on("connectionFailed", (e) => this.onConnectionFailed(e));
    this.controller.emitter.on("connectionDenied", (e) => this.onConnectionDenied(e));

    const authGameData = storage[AuthGameData.storageKey] as AuthGameData | undefined;
    if (!(authGameData?.local || authGameData?.remote)) {
      authSystem.addAuthListener((data) => {
        if (data.remote) {
          browser.setAuthData(data.remote);
        }
        storage[AuthGameData.storageKey] = data;
        this.sp.browser.setFocused(false);
        this.startClient();
      });

      authSystem.main();
    } else {
      this.startClient();
    }
  }

  private onConnectionFailed(e: ConnectionFailed) {
    this.logTrace("Connection failed");
  }

  private onConnectionDenied(e: ConnectionDenied) {
    this.logTrace("Connection denied: " + e.error);
  }

  private startClient() {
    // TODO: refactor netInfo into service
    netInfo.start();

    // TODO: refactor animDebugSystem into service
    animDebugSystem.init(settings["skymp5-client"]["animDebug"] as animDebugSystem.AnimDebugSettings);

    // TODO: refactor playerCombatSystem into service
    playerCombatSystem.start();

    this.establishConnectionConditional();
    this.ctor();
  }

  private ctor() {
    // TODO: refactor WorldView into service
    this.resetView();

    // TODO: refactor into service
    setupHooks();

    // TODO: refactor updateOwner into service
    updateOwner.setup();

    this.sp.printConsole('SkympClient ctor');
  }

  private establishConnectionConditional() {
    if (storage.targetIp !== targetIp || storage.targetPort !== targetPort) {
      storage.targetIp = targetIp;
      storage.targetPort = targetPort;

      this.logTrace(`Connecting to ${storage.targetIp}:${storage.targetPort}`);
      this.controller.lookupListener(networking.NetworkingService).connect(targetIp, targetPort);
    } else {
      this.logTrace('Reconnect is not required');
    }
  }

  private resetView() {
    const prevView: WorldView = storage.view as WorldView;
    const view = new WorldView();
    once('update', () => {
      if (prevView && prevView.destroy) {
        prevView.destroy();
        printConsole('Previous View destroyed');
      }
      storage.view = view;
    });
    on('update', () => {
      const singlePlayerService = this.controller.lookupListener(SinglePlayerService);
      if (!singlePlayerService.isSinglePlayer) {
        const modelSource = this.controller.lookupListener(RemoteServer);
        view.update(modelSource.getWorldModel());
      }
    });
  }
}
