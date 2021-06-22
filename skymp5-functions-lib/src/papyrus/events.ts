import { Mp, PapyrusObject } from '../types/mp';

export const register = (mp: Mp): void => {
  mp['onUiEvent'] = (pcFormId: number, uiEvent: Record<string, unknown>) => {
    // Server sometimes pass 0, I think serverside hot reload breaks something
    if (!pcFormId) return console.log('Plz reconnect');

    switch (uiEvent.type) {
      case 'cef::chat:send': {
        const text = uiEvent.data;
        if (typeof text === 'string') {
          const ac: PapyrusObject = { type: 'form', desc: mp.getDescFromId(pcFormId) };
          mp.callPapyrusFunction('global', 'GM_Main', '_OnChatInput', null, [ac, text]);
        }
      }
    }
  };
};
