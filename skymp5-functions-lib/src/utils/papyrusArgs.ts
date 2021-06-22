import { PapyrusValue, PapyrusObject } from '../types/mp';

const err = (index: number, x: unknown, expectedTypeName: string): never => {
  throw new TypeError(
    `The argument with index ${index} has value (${JSON.stringify(
      x
    )}) that doesn't meet the requirements of ${expectedTypeName}`
  );
};

const getArray = (args: PapyrusValue[], index: number, type: string): unknown[] => {
  const x = args[index];
  if (x === null || x === undefined) {
    return [];
  }
  return Array.isArray(x) && !x.filter((v) => typeof v !== type).length ? (x as unknown[]) : err(index, x, type + '[]');
};

export const getObject = (args: PapyrusValue[], index: number): PapyrusObject => {
  const x = args[index];
  return x && typeof x === 'object' && !Array.isArray(x) ? (x as PapyrusObject) : err(index, x, 'PapyrusObject');
};

export const getString = (args: PapyrusValue[], index: number): string => {
  const x = args[index];
  return typeof x === 'string' ? x : err(index, x, 'string');
};

export const getStringArray = (args: PapyrusValue[], index: number): string[] => {
  return getArray(args, index, 'string') as string[];
};

export const getNumber = (args: PapyrusValue[], index: number): number => {
  const x = args[index];
  return typeof x === 'number' ? x : err(index, x, 'number');
};

export const getBoolean = (args: PapyrusValue[], index: number): boolean => {
  const x = args[index];
  return typeof x === 'boolean' ? x : err(index, x, 'boolean');
};

export const getNumberArray = (args: PapyrusValue[], index: number): number[] => {
  return getArray(args, index, 'number') as number[];
};
