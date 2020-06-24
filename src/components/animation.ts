import { ObjectReference, Debug, hooks, printConsole } from 'skyrimPlatform';
import { Movement } from './movement';

export interface Animation {
    animEventName: string;
    numChanges: number;
}

export interface AnimationApplyState {
    lastNumChanges: number;
}

export let applyAnimation = (refr: ObjectReference, anim: Animation, state: AnimationApplyState) => {
    if (state.lastNumChanges === anim.numChanges) return;
    state.lastNumChanges = anim.numChanges;

    Debug.sendAnimationEvent(refr, anim.animEventName);
};

export class AnimationSource {
    constructor(refr: ObjectReference) {
        this.refrId = refr.getFormID();
        hooks.sendAnimationEvent.add({
            enter: (ctx) => {
            },
            leave: (ctx) => {
                if (ctx.selfId !== this.refrId) return;
                if (!ctx.animationSucceeded) return;
                this.onSendAnimationEvent(ctx.animEventName);
            }
        });
    }

    filterMovement(mov: Movement) {
        if (this.weapDrawnBlocker >= Date.now()) mov.isWeapDrawn = true;
        if (this.weapNonDrawnBlocker >= Date.now()) mov.isWeapDrawn = false;

        if (this.sneakBlocker === mov.isSneaking) this.sneakBlocker = null;
        else if (this.sneakBlocker === true) mov.isSneaking = true;
        else if (this.sneakBlocker === false) mov.isSneaking = false;

        return mov;
    }

    getAnimation(): Animation {
        let { numChanges, animEventName } = this;
        return { numChanges, animEventName };
    }

    private onSendAnimationEvent(animEventName: string) {
        if (ignoredAnims.has(animEventName)) return;

        if (animEventName.toLowerCase().includes('unequip')) {
            this.weapNonDrawnBlocker = Date.now() + 300;
            return;
        }
        if (animEventName.toLowerCase().includes('equip')) {
            this.weapDrawnBlocker = Date.now() + 300;
            return;
        }

        if (animEventName === 'SneakStart') {
            this.sneakBlocker = true;
            return;
        }
        if (animEventName === 'SneakStop') {
            this.sneakBlocker = false;
            return;
        }

        this.numChanges++;
        this.animEventName = animEventName;
    }

    private refrId = 0;
    private numChanges = 0;
    private animEventName = '';

    private weapNonDrawnBlocker = 0;
    private weapDrawnBlocker = 0;
    private sneakBlocker: boolean | null = null;
}

let ignoredAnims = new Set<string>([
    'moveStart',
    'moveStop',
    'turnStop',
    'CyclicCrossBlend',
    'CyclicFreeze',
    'TurnLeft',
    'TurnRightdddw'
]);