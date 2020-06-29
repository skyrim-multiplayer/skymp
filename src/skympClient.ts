import { on, once, printConsole, storage, Game, loadGame, SoulGem } from 'skyrimPlatform';
import { WorldView } from './view';
import { WorldModel, FormModel } from './model';
import { getMovement, Movement, Transform } from './components/movement';
import { AnimationSource, Animation, setupHooks } from './components/animation';
import * as networking from './networking';

const sendMovementRateMs = 130;

enum MsgType {
    CustomPacket = 1,
    UpdateMovement = 2,
    UpdateAnimation = 3
};

interface CreateActorMessage {
    type: 'createActor';
    idx: number;
    transform: Transform;
    isMe: boolean;
}

interface DestroyActorMessage {
    type: 'destroyActor';
    idx: number;
}

interface UpdateMovementMessage {
    t: MsgType.UpdateMovement;
    idx: number;
    data: Movement;
}

interface UpdateAnimationMessage {
    t: MsgType.UpdateAnimation;
    idx: number;
    data: Animation;
}

interface FormModelInfo extends FormModel {
    // ...
}

export class SkympClient {
    constructor() {
        this.helloWorld();
        this.resetView();
        setupHooks();

        networking.connect('127.0.0.1', 7777);

        networking.on('connectionFailed', () => {
            printConsole('Connection failed');
        });

        networking.on('connectionDenied', (err: string) => {
            printConsole('Connection denied: ', err);
        });

        networking.on('connectionAccepted', () => {
            this.forms = [];
            this.myActorIndex = -1;
        });

        networking.on('message', (msgAny: any) => {
            let msgType = msgAny.type || MsgType[msgAny.t];
            if (msgType !== 'UpdateMovement') printConsole(msgType);
            if (msgAny.type === 'createActor') {
                let msg = msgAny as CreateActorMessage;

                let i = msg.idx;
                if (this.forms.length <= i)
                    this.forms.length = i + 1;

                this.forms[i] = {
                    movement: {
                        pos: msg.transform.pos,
                        rot: msg.transform.rot,
                        worldOrCell: msg.transform.worldOrCell,
                        runMode: "Standing",
                        direction: 0,
                        isInJumpState: false,
                        isSneaking: false,
                        isBlocking: false,
                        isWeapDrawn: false
                    }
                };

                if (msg.isMe)
                    this.myActorIndex = i;

                // TODO: move to view
                if (msg.isMe) {
                    loadGame(msg.transform.pos, msg.transform.rot, msg.transform.worldOrCell);
                }
            }
            else if (msgAny.type === 'destroyActor') {
                let msg = msgAny as DestroyActorMessage;

                let i = msg.idx;
                this.forms[i] = null;

                if (this.myActorIndex === msg.idx) {
                    this.myActorIndex = -1;

                    // TODO: move to view
                    Game.quitToMainMenu();
                }
            }
            else if (msgAny.t === MsgType.UpdateMovement) {
                let msg = msgAny as UpdateMovementMessage;

                let i = msg.idx;
                this.forms[i].movement = msg.data;
                if (!this.forms[i].numMovementChanges) {
                    this.forms[i].numMovementChanges = 0;
                }
                this.forms[i].numMovementChanges++;
            }
            else if (msgAny.t === MsgType.UpdateAnimation) {
                let msg = msgAny as UpdateAnimationMessage;

                let i = msg.idx;
                this.forms[i].animation = msg.data;
            }
        });

        on('update', () => { this.sendInputs(); });
    }    

    private sendMovement() {    
        if (this.myActorIndex === -1) return;

        let now = Date.now();
        if (now - this.lastSendMovementMoment > sendMovementRateMs) {
            networking.send({ t: MsgType.UpdateMovement, data: getMovement(Game.getPlayer()), idx: this.myActorIndex }, false);
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
                networking.send({ t: MsgType.UpdateAnimation, data: anim, idx: this.myActorIndex }, false);
            }
        }
    }

    private sendInputs() {
        this.sendMovement();
        this.sendAnimation();
    }

    private getWorldModel(): WorldModel {
        return { forms: this.forms, playerCharacterFormIdx: this.myActorIndex };
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
        on('update', () => view.update(this.getWorldModel()));
    }

    private helloWorld() {
        for (let i = 0; i < 100; ++i) printConsole();
        printConsole('Hello Multiplayer');
    }

    private playerAnimSource?: AnimationSource;
    private lastSendMovementMoment = 0;
    private lastAnimationSent?: Animation;
    private forms = new Array<FormModelInfo>();
    private myActorIndex = -1;
}