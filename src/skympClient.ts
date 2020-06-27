import { on, once, printConsole, storage, Game, loadGame } from 'skyrimPlatform';
import { WorldView } from './view';
import { WorldModel, FormModel } from './model';
import { getMovement, Movement } from './components/movement';
import { AnimationSource } from './components/animation';
import * as networking from './networking';
import * as miscHacks from './miscHacks';

const sendMovementRateMs = 150;

enum MsgType {
    CustomPacket = 1,
    UpdateMovement = 2,
    UpdateAnimation = 3
};

interface CreateActorMessage {
    type: 'createActor';
    formId: number;
    mov: Movement;
    isMe: boolean;
}

interface DestroyActorMessage {
    type: 'createActor';
    formId: number;
}

interface UpdateMovementMessage {
    type: 'updMov';
    formId: number;
    mov: Movement;
}

interface FormModelInfo extends FormModel {
    serverFormId: number;
}

export class SkympClient {
    constructor() {
        this.helloWorld();
        this.resetView();
        miscHacks.setup();

        networking.connect('127.0.0.1', 7777);

        networking.on('message', (msgAny: any) => {
            printConsole(msgAny.type);
            if (msgAny.type === 'createActor') {
                let msg = msgAny as CreateActorMessage;

                let i = this.forms.indexOf(null);
                if (i === -1) {
                    this.forms.push(null);
                    i = this.forms.length - 1;
                }

                this.forms[i] = {
                    serverFormId: msg.formId,
                    movement: msg.mov
                };

                if (msg.isMe) {
                    this.myActorRemoteFormId = msg.formId;
                    this.myActorIndex = i;
                }

                // TODO: move to view
                if (msg.isMe) {
                    loadGame(msg.mov.pos, msg.mov.rot, msg.mov.worldOrCell);
                }
            }
            else if (msgAny.type === 'destroyActor') {
                let msg = msgAny as DestroyActorMessage;

                let i = this.forms.findIndex(form => form.serverFormId === msg.formId);
                if (i !== -1) {
                    this.forms[i] = null;
                }

                if (this.myActorRemoteFormId === msg.formId) {
                    this.myActorRemoteFormId = 0;
                    this.myActorIndex = -1;

                    // TODO: move to view
                    Game.quitToMainMenu();
                }
            }
            else if (msgAny.type === 'updMov') {
                let msg = msgAny as UpdateMovementMessage;

                let i = this.forms.findIndex(form => form.serverFormId === msg.formId);
                if (i !== -1) {
                    this.forms[i].movement = msg.mov;
                }
            }
        });

        on('update', () => { this.sendInputs(); });
    }    

    private sendMovement() {    
        let now = Date.now();
        if (now - this.lastSendMovementMoment > sendMovementRateMs) {
            networking.send({ t: MsgType.UpdateMovement, data: getMovement(Game.getPlayer()) }, false);
            this.lastSendMovementMoment = now;
        }
    }

    private sendInputs() {
        this.sendMovement();
    }

    private getWorldModel(): WorldModel {
        return { forms: this.forms, playerCharacterFormIdx: this.myActorIndex };
        /*let refr = Game.getCurrentConsoleRef() || Game.getPlayer();
        let refrId = refr.getFormID();

        if (!this.animSources.has(refrId)) {
            this.animSources.set(refrId, new AnimationSource(refr));
        }
        let animSource = this.animSources.get(refrId);

        let pc: FormModel = { 
            movement: getMovement(refr), 
            baseId: refr.getBaseObject().getFormID(),
            animation: animSource.getAnimation()
        };

        pc.movement.pos[0] += 128;
        pc.movement.pos[1] += 128;
        pc.movement.pos[2] += 0;

        pc.movement = animSource.filterMovement(pc.movement);

        return { forms: [pc], playerCharacterFormIdx: -1};*/
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

    private animSources = new Map<number, AnimationSource>();
    private lastSendMovementMoment = 0;
    private lastNumAnimChanges = 0;
    private forms = new Array<FormModelInfo>();
    private myActorRemoteFormId = 0;
    private myActorIndex = -1;
}