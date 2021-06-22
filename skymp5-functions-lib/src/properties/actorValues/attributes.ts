import { Mp } from '../../types/mp';
import { FunctionInfo } from '../../utils/functionInfo';
import { avUpdate, avUpdateDamage, avUpdateRestore } from './functions';

export const attributesList: Record<string, number> = {
  Health: 0x3e8,
  Magicka: 0x3e9,
  Stamina: 0x3ea,
};

export const register = (mp: Mp): void => {
  Object.keys(attributesList).forEach((avName) => {
    mp.makeProperty(`av${avName}`, {
      isVisibleByOwner: true,
      isVisibleByNeighbors: true,
      updateOwner: new FunctionInfo(avUpdate).getText({ avName }),
      updateNeighbor: new FunctionInfo(avUpdate).getText({ avName }),
    });
    mp.makeProperty(`av${avName}Damage`, {
      isVisibleByOwner: true,
      isVisibleByNeighbors: true,
      updateOwner: new FunctionInfo(avUpdateDamage).getText({ avName }),
      updateNeighbor: new FunctionInfo(avUpdate).getText({ avName }),
    });
    mp.makeProperty(`av${avName}Restore`, {
      isVisibleByOwner: true,
      isVisibleByNeighbors: true,
      updateOwner: new FunctionInfo(avUpdateRestore).getText({ avName }),
      updateNeighbor: new FunctionInfo(avUpdate).getText({ avName }),
    });
  });
};
