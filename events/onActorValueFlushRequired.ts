import { Attr, MP, EventName } from '../types';
import { utils } from '../utility';
import { actorValues } from '../properties';

declare const mp: MP;

export const initActorValueFlushRequiredEvent = () => {
	for (const attr of ['health', 'magicka', 'stamina'] as Attr[]) {
		mp.makeEventSource(
			('_onActorValueFlushRequired' + attr) as EventName,
			`
      const update = () => {
        const attr = "${attr}";
        const percent = ctx.sp.Game.getPlayer().getActorValuePercentage(attr);
        if (ctx.state.percent !== percent) {
          if (ctx.state.percent !== undefined && percent === 1) {
            ctx.sendEvent();
          }
          ctx.state.percent = percent;
        }
      };
      (async () => {
        while (1) {
          await ctx.sp.Utility.wait(0.667);
          update();
        }
      });
    `
		);
	}

	for (const attr of ['health', 'magicka', 'stamina'] as Attr[]) {
		utils.hook(('_onActorValueFlushRequired' + attr) as EventName, (pcFormId: number) => {
			actorValues.flushRegen(pcFormId, attr);
		});
	}
};
