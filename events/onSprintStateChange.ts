import { CTX } from '../platform';
import { actorValues } from '../properties';
import { getFunctionText, utils } from '../utility';
import { Attr, AttrAll, MP } from '../types';

declare const mp: MP;
declare const ctx: CTX;

export const initSprintStateChangeEvent = () => {
	mp.makeEventSource(
		'_onSprintStateChange',
		getFunctionText(() => {
			ctx.sp.on('update', () => {
				const isSprinting = ctx.sp.Game.getPlayer().isSprinting();
				if (ctx.state.isSprinting !== isSprinting) {
					if (ctx.state.isSprinting !== undefined) {
						ctx.sendEvent(isSprinting ? 'start' : 'stop');
					}
					ctx.state.isSprinting = isSprinting;
				}
			});
		})
	);

	const sprintAttr: Attr = 'stamina';
	const staminaReduce = -10;
	utils.hook('_onSprintStateChange', (pcFormId: number, newState: any) => {
		switch (newState) {
			case 'start':
				actorValues.set(pcFormId, `mp_${sprintAttr}drain` as AttrAll, 'base', -staminaReduce);
				const damageMod = actorValues.get(pcFormId, sprintAttr, 'damage');
				actorValues.set(pcFormId, sprintAttr, 'damage', damageMod - staminaReduce);
				break;
			case 'stop':
				actorValues.set(pcFormId, `mp_${sprintAttr}drain` as AttrAll, 'base', 0);
				break;
			default:
				break;
		}
	});
};
