import { FormModel, WorldModel } from './model';
import { ObjectReference, Game, Actor, MotionType, settings, printConsole, ActorBase, once, Utility } from 'skyrimPlatform';

import { applyMovement, NiPoint3 } from './components/movement';
import { applyAnimation } from './components/animation';
import { Look, applyLook, applyTints } from './components/look';

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
    constructor(pos: NiPoint3, refrId: number, private callback: () => void) {
        let refr = ObjectReference.from(Game.getFormEx(refrId));
        if (!refr || refr.getFormID() !== refrId) return;

        refr.setPosition(...pos).then(() => this.enable(refrId));
    }

    private enable(refrId: number) {
        let refr = ObjectReference.from(Game.getFormEx(refrId));
        if (!refr || refr.getFormID() !== refrId) return;

        refr.enable(false).then(() => this.resurrect(refrId));
    }

    private resurrect(refrId: number) {
        let refr = ObjectReference.from(Game.getFormEx(refrId));
        if (!refr || refr.getFormID() !== refrId) return;

        let ac = Actor.from(refr);
        if (ac) {
            return ac.resurrect().then(this.callback);
        }
        return refr.setMotionType(MotionType.Keyframed, true).then(this.callback);
    }
}

export class FormView implements View<FormModel> {
    update(model: FormModel) {
        if (!model.movement) {
            throw new Error('FormModel without Movement component, is it a mistake?');
        }
        
        const AADeleteWhenDoneTestJeremyRegular = 0x0010D13E;
        let base = Game.getFormEx(+model.baseId) || Game.getFormEx(this.getLookBasedBase()) || Game.getFormEx(AADeleteWhenDoneTestJeremyRegular);

        let refr = ObjectReference.from(Game.getFormEx(this.refrId));
        if (!refr || refr.getBaseObject().getFormID() !== base.getFormID()) {
            this.destroy();
            refr = Game.getPlayer().placeAtMe(base, 1, true, true);
            if (this.look) {
                let ac = Actor.from(refr);
                if (ac) applyTints(ac, this.look);
            }
            this.ready = false;
            new SpawnProcess(model.movement.pos, refr.getFormID(), () => this.ready = true);
        }
        this.refrId = refr.getFormID();

        if (!this.ready) return;
        this.applyAll(refr, model);
    }

    destroy() {
        let refr = ObjectReference.from(Game.getFormEx(this.refrId));
        if (refr) refr.delete();
    }

    private applyAll(refr: ObjectReference, model: FormModel) {

        // temp
        let ids = [0x000E40DF, 0x000E40DE];
        ids.forEach(id => {
            let item = Game.getFormEx(id);
            if (Actor.from(refr)) Actor.from(refr).equipItem(item, true, true);
            Game.getPlayer().equipItem(item, true, true);
        });
        // end temp

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

        // Apply look
        if (model.look) {
            if (!this.look || !objectEquals(model.look, this.look)) {
                this.look = model.look;
                this.lookBasedBaseId = 0;
            }
        }

        let actor = Actor.from(refr);
        if (actor) {
            actor.startDeferredKill();
            actor.setActorValue('health', 99999);
            actor.forceActorValue('health', 99999);
        }
    }

    private getLookBasedBase(): number {
        let base = ActorBase.from(Game.getFormEx(this.lookBasedBaseId));
        if (!base && this.look) {
            printConsole(11);
            this.lookBasedBaseId = applyLook(this.look).getFormID();
            printConsole(22);
        }
        return this.lookBasedBaseId;
    }

    private refrId = 0;
    private ready = false;
    private animState = { lastNumChanges: 0 };
    private movState = { lastNumChanges: 0 };
    private lookBasedBaseId = 0;
    private look?: Look;
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