import { Mp, PapyrusObject, PapyrusValue } from '../../types/mp';
import { getNumber } from '../../utils/papyrusArgs';

export const getKeywords = (mp: Mp, self: PapyrusObject): PapyrusObject[] => {
  const selfId = mp.getIdFromDesc(self.desc);
  const data = mp.lookupEspmRecordById(selfId);
  const kwda = data.record?.fields.find((x) => x.type === 'KWDA')?.data;
  const keywords: PapyrusObject[] = [];
  if (kwda) {
    const dataView = new DataView(kwda.buffer);
    for (let i = 0; i < dataView.byteLength; i += 4) {
      keywords.push({
        desc: mp.getDescFromId(dataView.getUint32(i, true)),
        type: 'espm',
      });
    }
  }
  return keywords;
};

export const getNumKeywords = (mp: Mp, self: PapyrusObject): number | undefined => {
  const selfId = mp.getIdFromDesc(self.desc);
  const data = mp.lookupEspmRecordById(selfId);
  const ksiz = data.record?.fields.find((x) => x.type === 'KSIZ')?.data;
  if (ksiz) {
    const dataView = new DataView(ksiz.buffer);
    return dataView.getUint32(0, true);
  }
  return;
};

export const getNthKeyword = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]): PapyrusObject | undefined => {
  const selfId = mp.getIdFromDesc(self.desc);
  const data = mp.lookupEspmRecordById(selfId);
  const index = getNumber(args, 0) - 1;
  const kwda = data.record?.fields.find((x) => x.type === 'KWDA')?.data;
  if (kwda) {
    let dataView = new DataView(kwda.buffer);
    return {
      desc: mp.getDescFromId(dataView.getUint32(index * 4, true)),
      type: 'espm',
    };
  }
  return;
};
