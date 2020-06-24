import { on, once, printConsole, storage, Game, hooks } from 'skyrimPlatform';
import { WorldView } from './view';
import { WorldModel, FormModel } from './model';
import { getMovement } from './components/movement';
import { AnimationSource } from './components/animation';
import * as networking from './networking';

let isIdle = (animEventName: string) => {
    return animEventName === 'MotionDrivenIdle'
        || (animEventName.startsWith('Idle') 
        && animEventName !== 'IdleStop' 
        && animEventName !== 'IdleForceDefaultState');
}

export class SkympClient {
    constructor() {
        for (let i = 0; i < 100; ++i) printConsole();
        printConsole('Hello Multiplayer');

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

        // Disable idle animations for 0xff actors
        hooks.sendAnimationEvent.add({
            enter: (ctx) => {
                if (ctx.selfId < 0xff000000) return;
                if (isIdle(ctx.animEventName)) ctx.animEventName = '';
            },
            leave: () => {
            }
        });
    }    

    private getWorldModel(): WorldModel {
        let refr = Game.getCurrentConsoleRef() || Game.getPlayer();
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

        return { forms: [pc], playerCharacterFormIdx: -1};
    }

    private animSources = new Map<number, AnimationSource>();
}