import { Counter } from '../logic/PlayerController';
import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';

declare const mp: Mp;

type State = Record<string, number> | undefined;

export class CounterProperty {
  static init() {
    mp.makeProperty('counter', {
      isVisibleByOwner: false,
      isVisibleByNeighbors: false,
      updateOwner: '',
      updateNeighbor: '',
    });
  }

  static get(actorId: number, counterName: Counter) {
    const current = mp.get(actorId, 'counter') as State ?? {};
    return current[counterName] ?? 0;
  }

  static set(actorId: number, counterName: Counter, newVal: number) {
    const current = mp.get(actorId, 'counter') as State ?? {};
    current[counterName] = newVal;
    mp.set(actorId, 'counter', current);
  }
}
