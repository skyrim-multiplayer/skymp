import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';

interface EvalState {
  neval: number;
}
interface EvalValue {
  n: number;
  f: string;
}
interface EvalCommand {
  id: number;
  f: string;
}

const execEvalCommand = (mp: Mp, current: EvalCommand) => {
  const prev = mp.get(current.id, 'eval');
  const n = prev ? prev.n + 1 : 1;
  mp.set(current.id, 'eval', { n, f: current.f });
};

let evalRunning = false;

const evalCommandList: EvalCommand[] = [];

const shiftEvalCommand = (mp: Mp) => {
  const current = evalCommandList.shift();
  if (current) {
    execEvalCommand(mp, current);
    setTimeout(() => {
      shiftEvalCommand(mp);
    }, 200);
  } else {
    evalRunning = false;
  }
};

export const evalClient = (mp: Mp, id: number, f: string) => {
  evalCommandList.push({ id, f });

  if (evalRunning) return;

  const current = evalCommandList.shift();
  if (current) {
    execEvalCommand(mp, current);
    setTimeout(() => {
      shiftEvalCommand(mp);
    }, 200);
    evalRunning = true;
  }
};

const evalUpdate = (ctx: Ctx<EvalState, EvalValue>) => {
  if (!ctx.value || ctx.state.neval === ctx.value.n) return;
  ctx.state.neval = ctx.value.n;
  eval(ctx.value.f);
};

export const register = (mp: Mp): void => {
  mp.makeProperty('eval', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: new FunctionInfo(evalUpdate).body,
    updateNeighbor: '',
  });
};
