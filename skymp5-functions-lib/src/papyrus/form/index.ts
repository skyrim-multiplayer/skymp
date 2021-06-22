import { Mp, PapyrusObject } from '../../types/mp';
import { uint8arrayToStringMethod } from '../../utils/helper';
import { getKeywords, getNthKeyword, getNumKeywords } from './keywords';
import { formType } from './type';

const getFormID = (mp: Mp, self: PapyrusObject) => mp.getIdFromDesc(self.desc);

const getName = (mp: Mp, self: PapyrusObject) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const data = mp.lookupEspmRecordById(selfId);
  const full = data.record?.fields.find((x) => x.type === 'FULL')?.data;
  const edid = data.record?.fields.find((x) => x.type === 'EDID')?.data;
  if (full) {
    const dataView = new DataView(full.buffer);
    // TODO: use localization table
  }
  if (edid) {
    return uint8arrayToStringMethod(edid);
  }
};

const getGoldValue = (mp: Mp, self: PapyrusObject) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const recordData = mp.lookupEspmRecordById(selfId);
  const data = recordData.record?.fields.find((x) => x.type === 'DATA')?.data;
  if (data) {
    const dataView = new DataView(data.buffer);
    return dataView.getUint32(0, true);
  }
  return -1;
};

const getWeight = (mp: Mp, self: PapyrusObject) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const recordData = mp.lookupEspmRecordById(selfId);
  const data = recordData.record?.fields.find((x) => x.type === 'DATA')?.data;
  if (data) {
    const dataView = new DataView(data.buffer);
    return dataView.getFloat32(4, true);
  }
  return -1;
};

const getType = (mp: Mp, self: PapyrusObject) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const data = mp.lookupEspmRecordById(selfId);
  return data.record?.type && formType[data.record?.type] ? formType[data.record?.type] : 0;
};

export const register = (mp: Mp): void => {
  mp.registerPapyrusFunction('method', 'Form', 'GetFormID', (self) => getFormID(mp, self));

  mp.registerPapyrusFunction('method', 'Form', 'GetName', (self) => getName(mp, self));
  mp.registerPapyrusFunction('method', 'Form', 'GetType', (self) => getType(mp, self));
  mp.registerPapyrusFunction('method', 'Form', 'GetGoldValue', (self) => getGoldValue(mp, self));
  mp.registerPapyrusFunction('method', 'Form', 'GetWeight', (self) => getWeight(mp, self));

  mp.registerPapyrusFunction('method', 'Form', 'GetKeywords', (self) => getKeywords(mp, self));
  mp.registerPapyrusFunction('method', 'Form', 'GetNumKeywords', (self) => getNumKeywords(mp, self));
  mp.registerPapyrusFunction('method', 'Form', 'GetNthKeyword', (self, args) => getNthKeyword(mp, self, args));
};
