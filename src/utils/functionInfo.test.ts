import { FunctionInfo } from './functionInfo';

describe('FunctionInfo', () => {
  it('should be able to get the body of an arrow function', () => {
    const f = (a: number, b: number) => {
      return a + b;
    };
    const functionInfo = new FunctionInfo(f);
    expect(functionInfo.body).toEqual('return a + b;');
  });

  it('should be able to get the body of an arrow function in a short form', () => {
    const f = (a: number, b: number) => a + b;
    const functionInfo = new FunctionInfo(f);
    expect(functionInfo.body).toEqual('return a + b;');
  });

  it('should be able to get the body of a normal function', () => {
    const f = function (a: number, b: number) {
      return a + b;
    };
    const functionInfo = new FunctionInfo(f);
    expect(functionInfo.body).toEqual('return a + b;');
  });
});
