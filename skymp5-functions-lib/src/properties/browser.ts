import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';

const browserVisibleUpdate = (ctx: Ctx<{ lastBrowserVisible: boolean }, boolean>) => {
  if (ctx.value === undefined || ctx.state.lastBrowserVisible === ctx.value) return;
  ctx.sp.browser.setVisible(ctx.value);
  ctx.state.lastBrowserVisible = ctx.value;
};

const browserFocusedUpdate = (ctx: Ctx<{ lastBrowserFocused: boolean }, boolean>) => {
  if (ctx.value === undefined || ctx.state.lastBrowserFocused === ctx.value) return;
  ctx.sp.browser.setFocused(ctx.value);
  ctx.state.lastBrowserFocused = ctx.value;
};

export const register = (mp: Mp): void => {
  mp.makeProperty('browserVisible', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: new FunctionInfo(browserVisibleUpdate).tryCatch(),
    updateNeighbor: '',
  });
  mp.makeProperty('browserFocused', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: new FunctionInfo(browserFocusedUpdate).tryCatch(),
    updateNeighbor: '',
  });
};
