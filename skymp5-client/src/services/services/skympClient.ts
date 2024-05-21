import {
  on,
  once,
  printConsole,
  settings,
  storage,
} from 'skyrimPlatform';
import * as networking from './networkingService';
import { RemoteServer } from './remoteServer';
import { setupHooks } from '../../sync/animation';
import { WorldView } from '../../view/worldView';
import { SinglePlayerService } from './singlePlayerService';
import { AuthGameData, authGameDataStorageKey } from '../../features/authModel';
import { ClientListener, CombinedController, Sp } from './clientListener';
import { ConnectionFailed } from '../events/connectionFailed';
import { ConnectionDenied } from '../events/connectionDenied';
import { ConnectionMessage } from '../events/connectionMessage';
import { CreateActorMessage } from '../messages/createActorMessage';
import { AuthAttemptEvent } from '../events/authAttemptEvent';
import { logTrace } from '../../logging';

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

    this.controller.emitter.on("createActorMessage", (e) => this.onActorCreateMessage(e));

    // TODO: refactor out very similar code in frontHotReloadService.ts
    const authGameData = storage[authGameDataStorageKey] as AuthGameData | undefined;

    const storageHasValidAuthGameData = authGameData?.local || authGameData?.remote;

    if (storageHasValidAuthGameData) {
      logTrace(this, `Recovered AuthGameData from storage, starting client`);
      this.startClient();
    } else {
      logTrace(this, `Unable to recover AuthGameData from storage, requesting auth`);

      // Next tick because we're in constructor of the service, AuthService may not be listening events yet
      this.controller.once("tick", () => {
        this.controller.emitter.emit("authNeeded", {});
      });
      this.controller.emitter.on("authAttempt", (e) => this.onAuthAttempt(e));
    }
  }

  private onAuthAttempt(e: AuthAttemptEvent) {
    logTrace(this, `Caught auth event`);

    storage[authGameDataStorageKey] = e.authGameData;

    this.startClient();

    // TODO: remove this when you will be able to see errors without console
    // this.sp.browser.setFocused(false);
  }

  private onActorCreateMessage(e: ConnectionMessage<CreateActorMessage>) {
    if (e.message.isMe) {
      this.sp.browser.setFocused(false);
    }
  }

  private onConnectionFailed(e: ConnectionFailed) {
    logTrace(this, "Connection failed");
  }

  private onConnectionDenied(e: ConnectionDenied) {
    logTrace(this, "Connection denied: " + e.error);
  }

  private startClient() {
    this.establishConnectionConditional();
    this.ctor();
  }

  private ctor() {
    // TODO: refactor WorldView into service
    this.resetView();

    // TODO: refactor into service
    setupHooks();

    this.sp.printConsole('SkympClient ctor');
  }

  private establishConnectionConditional() {
    if (storage.targetIp !== targetIp || storage.targetPort !== targetPort) {
      storage.targetIp = targetIp;
      storage.targetPort = targetPort;

      logTrace(this, `Connecting to`, storage.targetIp + ':' + storage.targetPort);
      this.controller.lookupListener(networking.NetworkingService).connect(targetIp, targetPort);
    } else {
      logTrace(this, 'Reconnect is not required');
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
