import * as networking from './networking';
import { FormModel } from './model';
import { MsgHandler } from './msgHandler';
import { ModelSource } from './modelSource';
import { SendTarget } from './sendTarget';
import * as messages from './messages';
import { Game, once, TESModPlatform, Cell, WorldSpace, printConsole, Utility, loadGame } from 'skyrimPlatform';
import * as loadGameManager from './loadGameManager';

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
        if (msg.look) {
            this.forms[i].look = msg.look;
        }

        if (msg.equipment) {
            this.forms[i].equipment = msg.equipment;
        }
        
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
                        loadGameManager.loadGame(msg.transform.pos, msg.transform.rot, msg.transform.worldOrCell);
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

    UpdateLook(msg: messages.UpdateLookMessage) {
        let i = msg.idx;
        this.forms[i].look = msg.data;
    }

    UpdateEquipment(msg: messages.UpdateEquipmentMessage) {
        let i = msg.idx;
        this.forms[i].equipment = msg.data;
    }

    handleConnectionAccepted() {
        this.forms = [];
        this.myActorIndex = -1;
    }

    handleDisconnect() {
    }

    setRaceMenuOpen(msg: messages.SetRaceMenuOpenMessage) {
        if (msg.open) {
            // wait 0.3s cause we can see visual bugs when teleporting
            // and showing this menu at the same time in onConnect
            once('update', () => Utility.wait(0.3).then(() => Game.showRaceMenu()));
        }
        else {
            // TODO: Implement closeMenu in SkyrimPlatform
        }
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