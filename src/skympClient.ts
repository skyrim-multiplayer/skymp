import { on, once, printConsole, storage, settings, Game, findConsoleCommand, SwitchRaceCompleteEvent, ActorBase, TESModPlatform } from 'skyrimPlatform';
import { WorldView } from './view';
import { getMovement} from './components/movement';
import { Look, Tint, getLook } from './components/look';
import { AnimationSource, Animation, setupHooks } from './components/animation';
import { MsgType } from './messages';
import { MsgHandler } from './msgHandler';
import { ModelSource } from './modelSource';
import { RemoteServer } from './remoteServer';
import { SendTarget } from './sendTarget';
import * as networking from './networking';
import * as sp from 'skyrimPlatform';

let handleMessage = (msgAny: any, handler: MsgHandler) => {
    let msgType = msgAny.type || MsgType[msgAny.t];
    let f = handler[msgType];
    if (msgType !== 'UpdateMovement') printConsole(msgType, msgAny);
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
                this.sendTarget.send({ t: MsgType.UpdateAnimation, data: anim }, false);
            }
        }
    }

    private sendLook() {
        let shown = sp['UI']['isMenuOpen']('RaceSex Menu'); 
        if (shown != this.isRaceSexMenuShown) {
            this.isRaceSexMenuShown = shown;
            if (!shown) {
                printConsole('Exited from race menu');

                let look = getLook(Game.getPlayer());
                this.sendTarget.send({ t: MsgType.UpdateLook, data: look }, true);
                for (let key in look) {
                    printConsole(`${key} = ${JSON.stringify(look[key])}`);
                }
            }
        }
    }

    private sendInputs() {
        this.sendMovement();
        this.sendAnimation();
        this.sendLook();
    }

    private resetRemoteServer() {
        let prevRemoteServer: RemoteServer = storage.remoteServer;
        let rs: RemoteServer;

        if (prevRemoteServer && prevRemoteServer.getWorldModel) {
            rs = prevRemoteServer;
            printConsole('Restore previous RemoteServer');
            
            // Keep previous RemoteServer, but update func implementations
            let newObj = new RemoteServer;
            for (let key in newObj) {
                if (typeof newObj[key] === 'function') rs[key] = newObj[key];
            }
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
    private isRaceSexMenuShown = false;
}

findConsoleCommand('showracemenu').execute = () => {
    printConsole('bope');
    return false;
};

// TODO: remove this
once('update', () => {
    Game.getPlayer().unequipAll();
});