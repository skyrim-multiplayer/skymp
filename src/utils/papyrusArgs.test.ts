import { getNumber, getObject, getString, getStringArray, getNumberArray } from './papyrusArgs';
import { PapyrusObject } from '../types/mp';

describe('getObject', () => {
  it('should be able to extract PapyrusObject from arguments', () => {
    const obj: PapyrusObject = {
      desc: '12eb7:Skyrim.esm',
      type: 'espm',
    };
    expect(getObject([obj], 0)).toEqual(obj);
  });

  it('should throw when bad value is provided', () => {
    expect(() => getObject([108], 0)).toThrowError(
      /The argument with index 0 has value \(108\) that doesn't meet the requirements of PapyrusObject/
    );
  });
});

describe('get primitive value', () => {
  it('should be able to extract a primitive value from arguments', () => {
    expect(getString([12, 'houe', null], 1)).toEqual('houe');
    expect(getNumber([12, 'houe', null], 0)).toEqual(12);
  });

  it('should throw when bad value is provided', () => {
    expect(() => getString([12, 'houe', null], 2)).toThrowError(
      /The argument with index 2 has value \(null\) that doesn't meet the requirements of string/
    );
    expect(() => getNumber([12, 'houe', null], 1)).toThrowError(
      /The argument with index 1 has value \("houe"\) that doesn't meet the requirements of number/
    );
  });
});

describe('get array value', () => {
  it('should be able to extract a string array from arguments', () => {
    expect(getStringArray([1, 2, 3, ['4'], 5, 6], 3)).toEqual(['4']);
    expect(getNumberArray(['1', '2', '3', [4], '5', '6'], 3)).toEqual([4]);
  });

  it('should throw when bad value is provided', () => {
    expect(() => getStringArray([1, 2, 3, ['4'], 5, 6], 0)).toThrowError(
      /The argument with index 0 has value \(1\) that doesn't meet the requirements of string\[\]/
    );
    expect(() => getNumberArray(['1', '2', '3', [4], '5', '6'], 0)).toThrowError(
      /The argument with index 0 has value \("1"\) that doesn't meet the requirements of number\[\]/
    );
  });

  it('should interpret null/undefined as an empty array', () => {
    expect(getStringArray([null], 0)).toEqual([]);
    expect(getStringArray([], 108)).toEqual([]);
    expect(getNumberArray([null], 0)).toEqual([]);
    expect(getNumberArray([], 108)).toEqual([]);
  });
});
