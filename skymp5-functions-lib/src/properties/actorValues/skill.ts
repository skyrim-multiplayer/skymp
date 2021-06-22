import { Mp } from '../../types/mp';
import { FunctionInfo } from '../../utils/functionInfo';
import { avUpdate, avUpdateDamage, avUpdateExp } from './functions';
import { skillList } from './skillList';

export const register = (mp: Mp): void => {
  Object.keys(skillList).forEach((avName) => {
    mp.makeProperty(`av${avName}`, {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(avUpdate).getText({ avName }),
      updateNeighbor: '',
    });
    mp.makeProperty(`av${avName}Exp`, {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(avUpdateExp).getText({ avName }),
      updateNeighbor: '',
    });
    mp.makeProperty(`av${avName}Damage`, {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(avUpdateDamage).getText({ avName }),
      updateNeighbor: '',
    });
  });
};
