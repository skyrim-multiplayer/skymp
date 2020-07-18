import { ObjectReference, Debug, hooks, Actor, printConsole } from 'skyrimPlatform';
import { Movement } from './movement';
import { applyWeapDrawn } from './movementApply';

export interface Animation {
    animEventName: string;
    numChanges: number;
}

export interface AnimationApplyState {
    lastNumChanges: number;
}


let isIdle = (animEventName: string) => {
    return animEventName === 'MotionDrivenIdle'
        || (animEventName.startsWith('Idle') 
        && animEventName !== 'IdleStop' 
        && animEventName !== 'IdleForceDefaultState');
}

let allowedIdles = new Array<[number, string]>();

export let applyAnimation = (refr: ObjectReference, anim: Animation, state: AnimationApplyState) => {
    if (state.lastNumChanges === anim.numChanges) return;
    state.lastNumChanges = anim.numChanges;

    if (isIdle(anim.animEventName)) {
        allowedIdles.push([refr.getFormID(), anim.animEventName]);
    }

    if (anim.animEventName === 'SkympFakeEquip') {
        let ac = Actor.from(refr);
        if (ac) applyWeapDrawn(ac, true);
    }
    else if (anim.animEventName === 'SkympFakeUnequip') {
        let ac = Actor.from(refr);
        if (ac) applyWeapDrawn(ac, false);
    }
    else {
        Debug.sendAnimationEvent(refr, anim.animEventName);
    }
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
            animEventName = 'SkympFakeUnequip';
        }
        else if (animEventName.toLowerCase().includes('equip')) {
            this.weapDrawnBlocker = Date.now() + 300;
            animEventName = 'SkympFakeEquip';
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
    'TurnRight'
]);

export let setupHooks = () => {
    hooks.sendAnimationEvent.add({
        enter: (ctx) => {
            // ShowRaceMenu forces this anim
            if (ctx.animEventName ==='OffsetBoundStandingPlayerInstant') {
                return ctx.animEventName = '';
            }
            
            // Disable idle animations for 0xff actors
            if (ctx.selfId < 0xff000000) return;
            if (isIdle(ctx.animEventName)) {
                let i = allowedIdles.findIndex((pair) => {
                    return pair[0] === ctx.selfId && pair[1] === ctx.animEventName;
                });
                i === -1 ? ctx.animEventName = '' : allowedIdles.splice(i, 1);
            }
        },
        leave: () => {

        }
    });
};