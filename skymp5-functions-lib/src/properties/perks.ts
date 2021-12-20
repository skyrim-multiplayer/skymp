import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';
import { isArrayEqual } from '../utils/helper';

// TODO: check isEqual works correct or not?
function perksUpdate(
  ctx: Ctx<{ lastPerks: number[] | undefined }, number[]>,
  isEqual: (arr1: any, arr2: any) => boolean
) {
  const ac = ctx.sp.Actor.from(ctx.refr as any);
  if (!ac) return;
  // !isEqual(ctx.state.lastPerks, ctx.value)
  if (ctx.value && ctx.value?.length !== ctx.state.lastPerks?.length) {
    const lastPerks = ctx.state.lastPerks ?? [];

    lastPerks
      .filter((x) => !ctx.value?.includes(x))
      .forEach((id) => {
        const newPerk = ctx.sp.Perk.from(ctx.sp.Game.getFormEx(id));
        if (ac.hasPerk(newPerk)) {
          ac.removePerk(newPerk);
        }
      });

    ctx.value.forEach((id) => {
      const newPerk = ctx.sp.Perk.from(ctx.sp.Game.getFormEx(id));
      if (!ac.hasPerk(newPerk)) {
        ac.addPerk(newPerk);
      }
    });

    ctx.state.lastPerks = ctx.value;
  }
}

export const register = (mp: Mp): void => {
  mp.makeProperty('perk', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: new FunctionInfo(perksUpdate).getText({ isEqual: isArrayEqual }),
    updateNeighbor: '',
  });
};
