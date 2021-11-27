import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';

declare const mp: Mp;

export class ChatProperty {
  static init() {
    mp.makeProperty('chat', {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(this.clientsideChatUpdate()).getText(),
      updateNeighbor: '',
    });
  }

  private static clientsideChatUpdate() {
    return (ctx: Ctx) => {
        
    };
  }
}
