import { hooks } from "skyrimPlatform";

let isIdle = (animEventName: string) => {
    return animEventName === 'MotionDrivenIdle'
        || (animEventName.startsWith('Idle') 
        && animEventName !== 'IdleStop' 
        && animEventName !== 'IdleForceDefaultState');
}

export let setup = () => {
    // Disable idle animations for 0xff actors
    hooks.sendAnimationEvent.add({
        enter: (ctx) => {
            if (ctx.selfId < 0xff000000) return;
            if (isIdle(ctx.animEventName)) ctx.animEventName = '';
        },
        leave: () => {

        }
    });
};