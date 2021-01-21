import { CTX } from '../platform';
import { actorValues } from '../properties';
import { getFunctionText, utils } from '../utility';
import { Attr, MP } from '../types';

declare const mp: MP;
declare const ctx: CTX;

export const initPowerAttacksEvent = () => {
	mp.makeEventSource(
		'_onPowerAttack',
		getFunctionText(() => {
			const next = ctx.sp.storage._api_onAnimationEvent;
			ctx.sp.storage._api_onAnimationEvent = {
				callback(...args: any[]) {
					const [serversideFormId, animEventName] = args;
					if (serversideFormId === 0x14 && animEventName.toLowerCase().includes('power')) {
						ctx.sendEvent(serversideFormId);
					}
					if (typeof next.callback === 'function') {
						next.callback(...args);
					}
				},
			};
		})
	);

	const sprintAttr: Attr = 'stamina';
	utils.hook('_onPowerAttack', (pcFormId: number) => {
		const damage = actorValues.get(pcFormId, sprintAttr, 'damage');
		const damageMod = -35;
		actorValues.set(pcFormId, sprintAttr, 'damage', damage + damageMod);
	});
};
