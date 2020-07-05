import * as networking from './networking';
import { FormModel } from './model';
import { MsgHandler } from './msgHandler';
import { ModelSource } from './modelSource';
import { SendTarget } from './sendTarget';
import * as messages from './messages';
import { loadGame, Game, once, TESModPlatform, Cell, WorldSpace, printConsole } from 'skyrimPlatform';

interface FormModelInfo extends FormModel {
    // ...
}

class SpawnTask {
    running = false;
}

export class RemoteServer implements MsgHandler, ModelSource, SendTarget {
    createActor(msg: messages.CreateActorMessage) {
        let i = msg.idx;
        if (this.forms.length <= i) this.forms.length = i + 1;
        
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
        
        if (msg.isMe) this.myActorIndex = i;
        
        // TODO: move to a separate module
        if (msg.isMe) {
            let task = new SpawnTask;
            once('update', () => {
                if (!task.running) {
                    task.running = true;
                    printConsole('Using moveRefrToPosition to spawn player');
                    TESModPlatform.moveRefrToPosition(
                        Game.getPlayer(), 
                        Cell.from(Game.getFormEx(msg.transform.worldOrCell)), 
                        WorldSpace.from(Game.getFormEx(msg.transform.worldOrCell)),
                        msg.transform.pos[0],
                        msg.transform.pos[1],
                        msg.transform.pos[2],
                        msg.transform.rot[0],
                        msg.transform.rot[1],
                        msg.transform.rot[2]
                    );
                }
            });
            once('tick', () => {
                once('tick', () => {
                    if (!task.running) {
                        task.running = true;
                        printConsole('Using loadGame to spawn player');
                        loadGame(msg.transform.pos, msg.transform.rot, msg.transform.worldOrCell);
                    }
                });
            });
        }
    }

    destroyActor(msg: messages.DestroyActorMessage) {
        let i = msg.idx;
        this.forms[i] = null;

        if (this.myActorIndex === msg.idx) {
            this.myActorIndex = -1;

            // TODO: move to a separate module
            Game.quitToMainMenu();
        }
    }

    UpdateMovement(msg: messages.UpdateMovementMessage) {
        let i = msg.idx;
        this.forms[i].movement = msg.data;
        if (!this.forms[i].numMovementChanges) {
            this.forms[i].numMovementChanges = 0;
        }
        this.forms[i].numMovementChanges++;
    }

    UpdateAnimation(msg: messages.UpdateAnimationMessage) {
        let i = msg.idx;
        this.forms[i].animation = msg.data;
    }

    handleConnectionAccepted() {
        this.forms = [];
        this.myActorIndex = -1;
    }

    handleDisconnect() {
    }

    getWorldModel() {
        return { forms: this.forms, playerCharacterFormIdx: this.myActorIndex };
    }

    getMyActorIndex() {
        return this.myActorIndex;
    }

    send(msg: any, reliable: boolean) {
        if (this.myActorIndex === -1) return;

        msg.idx = this.myActorIndex;
        networking.send(msg, reliable);
    }

    private forms = new Array<FormModelInfo>();
    private myActorIndex = -1;
}