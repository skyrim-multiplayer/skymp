import * as sp from 'skyrimPlatform';
import {
  Game,
  on,
  once,
  printConsole,
  settings,
  storage,
} from 'skyrimPlatform';
import * as netInfo from '../../debug/netInfoSystem';
import * as updateOwner from '../../gamemodeApi/updateOwner';
import * as networking from '../../networking';
import * as deathSystem from '../../sync/deathSystem';
import { HostStartMessage, HostStopMessage, MsgType } from '../../messages';
import { ModelSource } from '../../modelSource/modelSource';
import { MsgHandler } from '../../modelSource/msgHandler';
import { RemoteServer } from '../../modelSource/remoteServer';
import { SendTarget } from '../../modelSource/sendTarget';
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

interface AnyMessage {
  type?: string;
  t?: number;
}

const handleMessage = (msgAny: AnyMessage, handler_: MsgHandler) => {
  const msgType: string = msgAny.type || (MsgType as any)[msgAny.t as any];
  const handler = handler_ as unknown as Record<
    string,
    (m: AnyMessage) => void
  >;
  const f = handler[msgType];

  if (msgType === 'hostStart') {
    const msg = msgAny as HostStartMessage;
    const target = msg.target;

    let hosted = storage['hosted'];
    if (typeof hosted !== typeof []) {
      // if you try to switch to Set checkout .concat usage.
      // concat compiles but doesn't work as expected
      hosted = new Array<number>();
      storage['hosted'] = hosted;
    }

    if (!(hosted as Array<unknown>).includes(target)) {
      (hosted as Array<unknown>).push(target);
    }
  }

  if (msgType === 'hostStop') {
    const msg = msgAny as HostStopMessage;
    const target = msg.target;
    printConsole('hostStop', target.toString(16));

    const hosted = storage['hosted'] as Array<number>;
    if (typeof hosted === typeof []) {
      storage['hosted'] = hosted.filter((x) => x !== target);
    }
  }

  if (f && typeof f === 'function') handler[msgType](msgAny);
};

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

export const connectWhenICallAndNotWhenIImport = (): void => {
  if (storage.targetIp !== targetIp || storage.targetPort !== targetPort) {
    storage.targetIp = targetIp;
    storage.targetPort = targetPort;

    printConsole(`Connecting to ${storage.targetIp}:${storage.targetPort}`);
    networking.connect(targetIp, targetPort);
  } else {
    printConsole('Reconnect is not required');
  }
};

export class SkympClient extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

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

  get sendTarget(): SendTarget | undefined {
    return this.rs;
  }

  get msgHandler(): MsgHandler | undefined {
    return this.rs;
  }

  get modelSource(): ModelSource | undefined {
    return this.rs;
  }

  private startClient() {
    // TODO: subscribe to events in constructor, not here
    netInfo.start();
    animDebugSystem.init(settings["skymp5-client"]["animDebug"] as animDebugSystem.AnimDebugSettings);

    playerCombatSystem.start();
    once("update", () => authSystem.setPlayerAuthMode(false));
    connectWhenICallAndNotWhenIImport();
    this.ctor();
  }

  private ctor() {
    // TODO: subscribe to events in constructor, not here
    this.resetView();
    this.resetRemoteServer();
    setupHooks();
    updateOwner.setup();

    sp.printConsole('SkympClient ctor');

    networking.on('connectionFailed', () => {
      printConsole('Connection failed');
    });

    networking.on('connectionDenied', (err: Record<string, any> | string) => {
      printConsole('Connection denied: ', err);
    });

    networking.on('connectionAccepted', () => {
      const msgHandler = this.msgHandler;
      if (msgHandler === undefined) {
        return this.logError("this.msgHandler was undefined in networking.on('connectionAccepted')");
      }
      msgHandler.handleConnectionAccepted();
    });

    networking.on('disconnect', () => {
      const msgHandler = this.msgHandler;
      if (msgHandler === undefined) {
        return this.logError("this.msgHandler was undefined in networking.on('disconnect')");
      }
      msgHandler.handleDisconnect();
    });

    networking.on('message', (msgAny: Record<string, unknown> | string) => {
      netInfo.NetInfo.addReceivedPacketCount(1);
      handleMessage(
        msgAny as Record<string, unknown>,
        this.msgHandler as MsgHandler,
      );
    });

    once('update', () => {
      const player = Game.getPlayer();
      if (player) {
        deathSystem.makeActorImmortal(player);
      }
    });
  }

  private resetRemoteServer() {
    const prevRemoteServer: RemoteServer = storage.remoteServer as RemoteServer;
    let rs: RemoteServer;

    if (prevRemoteServer && (prevRemoteServer.getWorldModel as unknown)) {
      rs = prevRemoteServer;
      printConsole('Restore previous RemoteServer');

      // Keep previous RemoteServer, but update func implementations
      const newObj: Record<string, unknown> =
        new RemoteServer() as unknown as Record<string, unknown>;
      const rsAny: Record<string, unknown> = rs as unknown as Record<
        string,
        unknown
      >;
      for (const key in newObj) {
        if (typeof newObj[key] === 'function') rsAny[key] = newObj[key];
      }
    } else {
      rs = new RemoteServer();
      printConsole('Creating RemoteServer');
    }

    this.rs = rs;
    storage.remoteServer = rs;
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
      const singlePlayerService = this.controller.lookupListener("SinglePlayerService") as SinglePlayerService;
      if (!singlePlayerService.isSinglePlayer)
        view.update((this.modelSource as ModelSource).getWorldModel());
    });
  }

  private rs?: RemoteServer;
}
