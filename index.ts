import { MP } from './types';
import { initUtils, utils } from './utility';

// import init functions
import { initActorValue, initConsoleOutput, initIsDead, initSpawnPoint } from './properties';
import {
	initAnimationEvent,
	initActorValueFlushRequiredEvent,
	initBashEvent,
	initConsoleCommandEvent,
	initHitEvent,
	initPowerAttacksEvent,
	initSprintStateChangeEvent,
} from './events';
import { initDevCommands } from './systems';

declare const mp: MP;

// init creates events, properties
// "utility/utils",
initUtils();
// "events/onHit",
initHitEvent();

// "properties/isDead",
initIsDead();

// "events/onSprintStateChange",
initSprintStateChangeEvent();

// "events/onPowerAttack",
initPowerAttacksEvent();

// "events/onBash",
initBashEvent();

// "properties/consoleOutput",
initConsoleOutput();

// "properties/actorValues",
initActorValue();

// "events/onActorValueFlushRequired",
initActorValueFlushRequiredEvent();

// "properties/spawnSystem",
initSpawnPoint();

// "events/onConsoleCommand",
initConsoleCommandEvent();

// "systems/developerCommands"
initDevCommands();

// "events/onAnimationEvent"
initAnimationEvent();

utils.hook('onInit', (pcFormId: number) => {
	mp.onReinit(pcFormId);
});

const getDistance = (a: number[], b: number[]) => {
	return Math.sqrt(Math.pow(a[0] - b[0], 2) + Math.pow(a[1] - b[1], 2) + Math.pow(a[2] - b[2], 2));
};

utils.hook('onUiEvent', (formId: number, msg: Record<string, unknown>) => {
	switch (msg.type) {
		case 'chatMessage':
			const myName = mp.get(formId, 'appearance').name;
			const myPos: number[] = mp.get(formId, 'pos');
			let neighbors: number[] = mp.get(formId, 'neighbors');
			neighbors = neighbors.filter((x) => mp.get(x, 'type') === 'MpActor');
			neighbors.forEach((neiFormId) => {
				const pos: number[] = mp.get(neiFormId, 'pos');
				const distance = getDistance(myPos, pos);
				if (distance >= 0 && distance < 1000) {
					mp.sendUiMessage(neiFormId, {
						pageIndex: msg.pageIndex,
						text: msg.text,
						name: myName,
						tagIndex: 0,
						type: 'chatMessage',
					});
					utils.log('Chat message handled', msg);
				}
			});
			break;
	}
});
