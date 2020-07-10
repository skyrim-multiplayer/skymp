import { on, once, printConsole, storage, settings, Game } from 'skyrimPlatform';
import { WorldView } from './view';
import { getMovement} from './components/movement';
import { AnimationSource, Animation, setupHooks } from './components/animation';
import { MsgType } from './messages';
import { MsgHandler } from './msgHandler';
import { ModelSource } from './modelSource';
import { RemoteServer } from './remoteServer';
import { SendTarget } from './sendTarget';
import * as networking from './networking';

let handleMessage = (msgAny: any, handler: MsgHandler) => {
    let msgType = msgAny.type || MsgType[msgAny.t];
    if (msgType !== 'UpdateMovement') printConsole(msgType, msgAny);

    let f = handler[msgType];
    if (f && typeof f === 'function') handler[msgType](msgAny);
};

for (let i = 0; i < 100; ++i) printConsole();
printConsole('Hello Multiplayer');
printConsole('settings:', settings['skymp5-client']);

let targetIp = settings['skymp5-client']['server-ip'];
let targetPort = settings['skymp5-client']['server-port'];;

if (storage.targetIp !== targetIp || storage.targetPort !== targetPort) {
    storage.targetIp = targetIp;
    storage.targetPort = targetPort;

    printConsole(`Connecting to ${storage.targetIp}:${storage.targetPort}`);
    networking.connect(targetIp, targetPort);
}
else {
    printConsole('Reconnect is not required');
}

export class SkympClient {
    constructor() {
        this.resetView();
        this.resetRemoteServer();
        setupHooks();

        networking.on('connectionFailed', () => {
            printConsole('Connection failed');
        });

        networking.on('connectionDenied', (err: string) => {
            printConsole('Connection denied: ', err);
        });

        networking.on('connectionAccepted', () => {
            this.msgHandler.handleConnectionAccepted();
        });

        networking.on('disconnect', () => {
            this.msgHandler.handleDisconnect();
        });

        networking.on('message', (msgAny: any) => {
            handleMessage(msgAny, this.msgHandler);
        });

        on('update', () => { this.sendInputs(); });
    }    

    private sendMovement() { 
        const sendMovementRateMs = 130;   
        let now = Date.now();
        if (now - this.lastSendMovementMoment > sendMovementRateMs) {
            this.sendTarget.send({ t: MsgType.UpdateMovement, data: getMovement(Game.getPlayer()) }, false);
            this.lastSendMovementMoment = now;
        }
    }

    private sendAnimation() {
        if (!this.playerAnimSource) {
            this.playerAnimSource = new AnimationSource(Game.getPlayer());
        }
        let anim = this.playerAnimSource.getAnimation();
        if (!this.lastAnimationSent || anim.numChanges !== this.lastAnimationSent.numChanges) {
            if (anim.animEventName !== '') {
                this.lastAnimationSent = anim;
                this.sendTarget.send({ t: MsgType.UpdateAnimation, data: anim}, false);
            }
        }
    }

    private sendInputs() {
        this.sendMovement();
        this.sendAnimation();
    }

    private resetRemoteServer() {
        let prevRemoteServer: RemoteServer = storage.remoteServer;
        let rs: RemoteServer;

        if (prevRemoteServer && prevRemoteServer.getWorldModel) {
            rs = prevRemoteServer;
            printConsole('Restore previous RemoteServer');
        }
        else {
            rs = new RemoteServer;
            printConsole('Creating RemoteServer');
        }

        this.sendTarget = rs;
        this.msgHandler = rs;
        this.modelSource = rs
        storage.remoteServer = rs;
    }

    private resetView() {
        let prevView: WorldView = storage.view;
        let view = new WorldView;
        once('update', () => {
            if (prevView && prevView.destroy) {
                prevView.destroy();
                printConsole('Previous View destroyed');
            }
            storage.view = view;
        });
        on('update', () => view.update(this.modelSource.getWorldModel()));
    }

    private playerAnimSource?: AnimationSource;
    private lastSendMovementMoment = 0;
    private lastAnimationSent?: Animation;
    private msgHandler?: MsgHandler;
    private modelSource?: ModelSource;
    private sendTarget?: SendTarget;
}