import { on, once, printConsole, storage, Game } from 'skyrimPlatform';
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
    if (msgType !== 'UpdateMovement') printConsole(msgType);

    let f = handler[msgType];
    if (f && typeof f === 'function') handler[msgType](msgAny);
};

export class SkympClient {
    constructor() {
        this.helloWorld();
        this.resetView();
        setupHooks();
        let remoteServer = new RemoteServer;
        this.sendTarget = remoteServer;
        this.msgHandler = remoteServer;
        this.modelSource = remoteServer;

        networking.connect('127.0.0.1', 7777);

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

    private helloWorld() {
        for (let i = 0; i < 100; ++i) printConsole();
        printConsole('Hello Multiplayer');
    }

    private playerAnimSource?: AnimationSource;
    private lastSendMovementMoment = 0;
    private lastAnimationSent?: Animation;
    private msgHandler?: MsgHandler;
    private modelSource?: ModelSource;
    private sendTarget?: SendTarget;
}