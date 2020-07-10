import { FormModel, WorldModel } from './model';
import { ObjectReference, Game, Actor, MotionType, settings } from 'skyrimPlatform';

import { applyMovement, NiPoint3 } from './components/movement';
import { applyAnimation } from './components/animation';

export interface View<T> {
    update(model: T);
    destroy();
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
        
        let base = Game.getFormEx(+model.baseId) || Game.getPlayer().getBaseObject();

        let refr = ObjectReference.from(Game.getFormEx(this.refrId));
        if (!refr || refr.getBaseObject().getFormID() !== base.getFormID()) {
            this.destroy();
            refr = Game.getPlayer().placeAtMe(base, 1, true, true);
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
        if (actor) {
            actor.startDeferredKill();
            actor.setActorValue('health', 99999);
            actor.forceActorValue('health', 99999);
        }
    }

    private refrId = 0;
    private ready = false;
    private animState = { lastNumChanges: 0 };
    private movState = { lastNumChanges: 0 };
}

export class WorldView implements View<WorldModel> {
    update(model: WorldModel) {
        this.resize(model.forms.length);

        const showMe = settings['skymp5-client']['show-me'];

        model.forms.forEach((form, i) => {
            if (!form || (model.playerCharacterFormIdx === i && !showMe)) {
                return this.destroyForm(i);
            }

            let realPos: NiPoint3;
            if (model.playerCharacterFormIdx === i && form.movement) {
                realPos = form.movement.pos;
                form.movement.pos = [realPos[0] + 128, realPos[1] + 128, realPos[2]];
            }
            this.updateForm(form, i);
            if (model.playerCharacterFormIdx === i && form.movement) {
                form.movement.pos = realPos;
            }
        });
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
}