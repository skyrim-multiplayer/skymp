import { MP } from '../types';
import { genClientFunction, utils } from '../utility';
import { CTX } from '../platform';
import { consoleOutput } from '../properties';
import { currentActor, TRACE_ANIMATION } from '../constants';

declare const mp: MP;
declare const ctx: CTX;

export const initAnimationEvent = () => {
	if (TRACE_ANIMATION) {
		mp.makeEventSource(
			'_onAnimationEvent',
			genClientFunction(() => {
				const next = ctx.sp.storage._api_onAnimationEvent;
				ctx.sp.storage._api_onAnimationEvent = {
					callback(...args: any) {
						const [serversideFormId, animEventName] = args;
						ctx.sendEvent(serversideFormId, animEventName);
						if (typeof next.callback === 'function') {
							next.callback(...args);
						}
					},
				};
			}, {})
		);

		/**
		 * on trigger animation event print animation name
		 */
		utils.hook('_onAnimationEvent', (pcFormId, serversideFormId, animEventName) => {
			if (serversideFormId !== currentActor) return;
			consoleOutput.print(pcFormId, animEventName);
		});
	}
};
