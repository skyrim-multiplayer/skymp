import { FormModel, WorldModel } from './model';
import { ObjectReference, Game, Actor, MotionType, settings, printConsole, ActorBase, once, on, Utility, worldPointToScreenPoint } from 'skyrimPlatform';
import * as sp from "skyrimPlatform";

import { applyMovement, NiPoint3 } from './components/movement';
import { applyAnimation } from './components/animation';
import { Look, applyLook, applyTints } from './components/look';
import { applyEquipment, isBadMenuShown } from './components/equipment';

export interface View<T> {
    update(model: T);
    destroy();
}

// https://stackoverflow.com/questions/201183/how-to-determine-equality-for-two-javascript-objects
function objectEquals(x, y) {
    'use strict';

    if (x === null || x === undefined || y === null || y === undefined) { return x === y; }
    // after this just checking type of one would be enough
    if (x.constructor !== y.constructor) { return false; }
    // if they are functions, they should exactly refer to same one (because of closures)
    if (x instanceof Function) { return x === y; }
    // if they are regexps, they should exactly refer to same one (it is hard to better equality check on current ES)
    if (x instanceof RegExp) { return x === y; }
    if (x === y || x.valueOf() === y.valueOf()) { return true; }
    if (Array.isArray(x) && x.length !== y.length) { return false; }

    // if they are dates, they must had equal valueOf
    if (x instanceof Date) { return false; }

    // if they are strictly equal, they both need to be object at least
    if (!(x instanceof Object)) { return false; }
    if (!(y instanceof Object)) { return false; }

    // recursive object equality check
    var p = Object.keys(x);
    return Object.keys(y).every(function (i) { return p.indexOf(i) !== -1; }) &&
        p.every(function (i) { return objectEquals(x[i], y[i]); });
}

class SpawnProcess {
    constructor(look: Look, pos: NiPoint3, refrId: number, private callback: () => void) {
        let refr = ObjectReference.from(Game.getFormEx(refrId));
        if (!refr || refr.getFormID() !== refrId) return;

        refr.setPosition(...pos).then(() => this.enable(look, refrId));
    }

    private enable(look: Look, refrId: number) {
        let refr = ObjectReference.from(Game.getFormEx(refrId));
        if (!refr || refr.getFormID() !== refrId) return;

        let ac = Actor.from(refr);
        if (look && ac) applyTints(ac, look);
        refr.enable(false).then(() => this.resurrect(look, refrId));
    }

    private resurrect(look: Look, refrId: number) {
        let refr = ObjectReference.from(Game.getFormEx(refrId));
        if (!refr || refr.getFormID() !== refrId) return;

        let ac = Actor.from(refr);
        if (ac) {
            return ac.resurrect().then(() => {
                this.callback();
            });
        }
        return refr.setMotionType(MotionType.Keyframed, true).then(this.callback);
    }
}

let getTime = () => Utility.getCurrentGameTime() * 60 * 60 * 1000;

export class FormView implements View<FormModel> {
    update(model: FormModel) {
        if (!model.movement) {
            throw new Error('FormModel without Movement component, is it a mistake?');
        }

        // Apply look before base form selection to prevent double-spawn
        if (model.look) {
            if (!this.look || !objectEquals(model.look, this.look)) {
                this.look = model.look;
                this.lookBasedBaseId = 0;
            }
        }
        
        const AADeleteWhenDoneTestJeremyRegular = 0x0010D13E;
        let base = Game.getFormEx(+model.baseId) || Game.getFormEx(this.getLookBasedBase()) || Game.getFormEx(AADeleteWhenDoneTestJeremyRegular);

        let refr = ObjectReference.from(Game.getFormEx(this.refrId));
        let respawnRequired = !refr || refr.getBaseObject().getFormID() !== base.getFormID();

        if (respawnRequired) {
            this.destroy();
            refr = Game.getPlayer().placeAtMe(base, 1, true, true);
            this.ready = false;
            new SpawnProcess(this.look, model.movement.pos, refr.getFormID(), () => this.ready = true);
            if (model.look && model.look.name) refr.setDisplayName('' + model.look.name, true);
        }
        this.refrId = refr.getFormID();

        if (!this.ready) return;
        this.applyAll(refr, model);
    }

    destroy() {
        this.isOnScreen = false;
        let refr = ObjectReference.from(Game.getFormEx(this.refrId));
        if (refr) refr.delete();
    }

    private applyAll(refr: ObjectReference, model: FormModel) {
        let forcedWeapDrawn: boolean | null = null;

        if (model.animation) {
            if (model.animation.animEventName === 'SkympFakeUnequip') {
                forcedWeapDrawn = false;
            }
            else if (model.animation.animEventName === 'SkympFakeEquip') {
                forcedWeapDrawn = true;
            }
        }

        if (model.movement) {
            if (+model.numMovementChanges !== this.movState.lastNumChanges) {

                let backup = model.movement.isWeapDrawn;
                if (forcedWeapDrawn === true || forcedWeapDrawn === false) {
                    model.movement.isWeapDrawn = forcedWeapDrawn;
                }
                applyMovement(refr, model.movement);
                model.movement.isWeapDrawn = backup;

                this.movState.lastNumChanges = +model.numMovementChanges;
            }
        }
        if (model.animation) applyAnimation(refr, model.animation, this.animState);

        let actor = Actor.from(refr);
        if (actor && !Game.getPlayer().getAnimationVariableBool('bInJumpState')) {
            let pcWorldOrCell = Game.getPlayer().getWorldSpace() || Game.getPlayer().getParentCell();
            if (pcWorldOrCell) {
                let id = pcWorldOrCell.getFormID();
                if (this.lastPcWorldOrCell && id !== this.lastPcWorldOrCell) {
                    // Redraw tints if PC world/cell changed
                    this.isOnScreen = false;
                }
                this.lastPcWorldOrCell = id;
            }

            let headPos = [
                sp['NetImmerse'].GetNodeWorldPositionX(actor, 'NPC Head [Head]', false),
                sp['NetImmerse'].GetNodeWorldPositionY(actor, 'NPC Head [Head]', false),
                sp['NetImmerse'].GetNodeWorldPositionZ(actor, 'NPC Head [Head]', false)
            ];
            let [screenPoint] = worldPointToScreenPoint(headPos);
            let isOnScreen = screenPoint[0] > 0 && screenPoint[1] > 0 && screenPoint [2] > 0 && screenPoint[0] < 1 && screenPoint[1] < 1 && screenPoint [2] < 1;
            if (isOnScreen != this.isOnScreen) {
                this.isOnScreen = isOnScreen;
                if (isOnScreen) {
                    actor.queueNiNodeUpdate();
                    Game.getPlayer().queueNiNodeUpdate();
                }
            }
        }

        if (model.equipment) {
            let isShown = isBadMenuShown();
            if (this.eqState.isBadMenuShown !== isShown) {
                this.eqState.isBadMenuShown = isShown; 
                if (!isShown) this.eqState.lastNumChanges = -1;
            }
            if (this.eqState.lastNumChanges !== model.equipment.numChanges) {
                this.eqState.lastNumChanges = model.equipment.numChanges;
                let ac = Actor.from(refr);
                if (ac) applyEquipment(ac, model.equipment);
            }
        }
    }

    private getLookBasedBase(): number {
        let base = ActorBase.from(Game.getFormEx(this.lookBasedBaseId));
        if (!base && this.look) {
            this.lookBasedBaseId = applyLook(this.look).getFormID();
        }
        return this.lookBasedBaseId;
    }

    private refrId = 0;
    private ready = false;
    private animState = { lastNumChanges: 0 };
    private movState = { lastNumChanges: 0 };
    private eqState = { lastNumChanges: 0, isBadMenuShown: false };
    private lookBasedBaseId = 0;
    private look?: Look;
    private isOnScreen = false;
    private lastPcWorldOrCell = 0;
}

export class WorldView implements View<WorldModel> {
    constructor() {
        // Work around showRaceMenu issue
        // Default nord in Race Menu will have very ugly face
        // If other players are spawning when we show this menu
        once('update', () => {
            // Wait 1s game time (time spent in Race Menu isn't counted)
            Utility.wait(1).then(() => {
                this.allowUpdate = true;
                printConsole('Update is now allowed');
            });
        });
    }

    update(model: WorldModel) {
        if (!this.allowUpdate) return;

        this.resize(model.forms.length);

        const showMe = settings['skymp5-client']['show-me'];

        let toDestroy = [];

        model.forms.forEach((form, i) => {
            if (!form || (model.playerCharacterFormIdx === i && !showMe)) {
                return this.destroyForm(i);
            }

            let realPos: NiPoint3;
            if (model.playerCharacterFormIdx === i && form.movement) {
                realPos = form.movement.pos;
                form.movement.pos = [realPos[0] + 128, realPos[1] + 128, realPos[2]];
            }
            try {
                this.updateForm(form, i);
            }
            catch(err) {
                if (err.message.includes('needs to be respawned')) {
                    toDestroy.push(i);
                    printConsole('destroying');
                }
                else {
                    throw err;
                }
            }
            if (model.playerCharacterFormIdx === i && form.movement) {
                form.movement.pos = realPos;
            }
        });

        toDestroy.forEach(i => this.destroyForm(i));
    }

    private updateForm(form: FormModel, i: number) {
        if (!this.formViews[i]) this.formViews[i] = new FormView();
        this.formViews[i].update(form);
    }

    private destroyForm(i: number) {
        if (!this.formViews[i]) return;
        this.formViews[i].destroy();
        this.formViews[i] = undefined;
    }

    private resize(newSize: number) {
        if (this.formViews.length > newSize) {
            this.formViews.slice(newSize).forEach(v => v && v.destroy());
        }
        this.formViews.length = newSize;
    }

    destroy() {
        this.resize(0);
    }

    private formViews = new Array<FormView>();
    private allowUpdate = false;
}