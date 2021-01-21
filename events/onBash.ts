import { currentActor } from '../constants';
import { actorValues } from '../properties';
import { MP, Attr } from '../types';
import { genClientFunction, utils } from '../utility';
import { CTX } from '../platform';

declare const mp: MP;
declare const ctx: CTX;

export const initBashEvent = () => {
	mp.makeEventSource(
		'_onBash',
		genClientFunction(
			() => {
				const next = ctx.sp.storage._api_onAnimationEvent;
				ctx.sp.storage._api_onAnimationEvent = {
					callback(...args: any[]) {
						const [serversideFormId, animEventName] = args;
						if (serversideFormId === currentActor && animEventName.toLowerCase().includes('bash')) {
							ctx.sendEvent(serversideFormId);
						}
						if (typeof next.callback === 'function') {
							next.callback(...args);
						}
					},
				};
			},
			{ currentActor }
		)
	);

	/**
	 * on trigger bash event reduce actor stamina
	 */
	const sprintAttr: Attr = 'stamina';
	utils.hook('_onBash', (pcFormId: number) => {
		const damage = actorValues.get(pcFormId, sprintAttr, 'damage');
		const damageMod = -35;
		actorValues.set(pcFormId, sprintAttr, 'damage', damage + damageMod);
	});
};
