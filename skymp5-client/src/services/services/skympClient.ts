import * as sp from 'skyrimPlatform';
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
import { ModelSource } from '../../modelSource/modelSource';
import { RemoteServer } from './remoteServer';
import { setupHooks } from '../../sync/animation';
import * as animDebugSystem from '../../debug/animDebugSystem';
import { WorldView } from '../../view/worldView';
import { SinglePlayerService } from './singlePlayerService';
import * as authSystem from "../../features/authSystem";
import * as playerCombatSystem from "../../sweetpie/playerCombatSystem";
import { AuthGameData } from '../../features/authModel';
import { Transform } from '../../sync/movement';
import * as browser from "../../features/browser";
import { ClientListener, CombinedController, Sp } from './clientListener';
import { SpApiInteractor } from '../spApiInteractor';
import { ConnectionFailed } from '../events/connectionFailed';
import { ConnectionDenied } from '../events/connectionDenied';
import { ConnectionAccepted } from '../events/connectionAccepted';
import { ConnectionDisconnect } from '../events/connectionDisconnect';
import { ConnectionMessage } from '../events/connectionMessage';

interface AnyMessage {
  type?: string;
  t?: number;
}

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

      authSystem.main(settings["skymp5-client"]["lobbyLocation"] as Transform);
    } else {
      this.startClient();
    }
  }

  get modelSource(): ModelSource | undefined {
    return this.rs;
  }

  private onConnectionFailed(e: ConnectionFailed) {
    this.logTrace("Connection failed");
  }

  private onConnectionDenied(e: ConnectionDenied) {
    this.logTrace("Connection denied: " + e.error);
  }

  private startClient() {
    // TODO: subscribe to events in constructor, not here
    netInfo.start();
    animDebugSystem.init(settings["skymp5-client"]["animDebug"] as animDebugSystem.AnimDebugSettings);

    playerCombatSystem.start();
    // once("update", () => authSystem.setPlayerAuthMode(false));
    this.establishConnectionConditional();
    this.ctor();
  }

  private ctor() {
    printConsole("CTOR")

    // TODO: subscribe to events in constructor, not here
    this.resetView();
    setupHooks();
    updateOwner.setup();

    this.sp.printConsole('SkympClient ctor');
  }

  private establishConnectionConditional() {
    if (storage.targetIp !== targetIp || storage.targetPort !== targetPort) {
      storage.targetIp = targetIp;
      storage.targetPort = targetPort;

      this.logTrace(`Connecting to ${storage.targetIp}:${storage.targetPort}`);
      SpApiInteractor.makeController().lookupListener(networking.NetworkingService).connect(targetIp, targetPort);
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
        const modelSource = this.modelSource;
        if (modelSource === undefined) {
          return this.logError("modelSource was undefined");
        }
        view.update(modelSource.getWorldModel());
      }
    });
  }

  private rs?: RemoteServer;
}
