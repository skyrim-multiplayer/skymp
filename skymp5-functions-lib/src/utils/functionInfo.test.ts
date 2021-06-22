import { FunctionInfo } from './functionInfo';

// TODO: remake tests to use try catch block
describe('FunctionInfo', () => {
  it('should be able to get the body of an arrow function', () => {
    const f = (a: number, b: number) => {
      return a + b;
    };
    const functionInfo = new FunctionInfo(f);
    expect(functionInfo.body).toEqual(`
    try {
      ${'return a + b;'}
    } catch(err) {
      ctx.sp.printConsole('[CTX ERROR]', err);
    }`);
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

  it('should be able to get the text with number argument of a normal function', () => {
    const f = function (a: number, b: number, c: number) {
      return a + b + c;
    };
    const functionInfo = new FunctionInfo(f);
    expect(functionInfo.getText({ c: 10 })).toEqual(`const c = 10;\nreturn a + b + c;`);
  });

  it('should be able to get the text with boolean argument of a normal function', () => {
    const f = function (a: number, b: number, c: boolean) {
      return a + b;
    };
    const functionInfo = new FunctionInfo(f);
    expect(functionInfo.getText({ c: false })).toEqual(`const c = false;\nreturn a + b;`);
  });

  it('should be able to get the text with string argument of a normal function', () => {
    const f = function (a: number, b: number, c: string) {
      return a + b;
    };
    const functionInfo = new FunctionInfo(f);
    expect(functionInfo.getText({ c: 'hello world' })).toEqual(`const c = 'hello world';\nreturn a + b;`);
  });

  it('should be able to get the text with number array argument of a normal function', () => {
    const f = function (a: number, b: number, c: number[]) {
      return a + b + c[2];
    };
    const functionInfo = new FunctionInfo(f);
    expect(functionInfo.getText({ c: [1, 2, 3] })).toEqual(`const c = [1,2,3];\nreturn a + b + c[2];`);
  });

  it('should be able to get the text with function argument of a normal function', () => {
    const f = function (a: number, b: number, c: Function) {
      return a + b;
    };
    const functionInfo = new FunctionInfo(f);
    expect(functionInfo.getText({ c: () => 'function argument ' + 'ready' })).toEqual(
      `const c = function () { return 'function argument ' + 'ready'; };\nreturn a + b;`
    );
  });
});
