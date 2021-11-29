export class FunctionInfo<F extends { toString: () => string }> {
  public constructor(private f: F) {}

  get text(): string {
    return `
        try {
          ${this.getTextWithoutErrorHandling()}
        } catch(err) {
          ctx.sp.printConsole('[CTX ERROR]', err, '\\n', ${this.f});
        }`;
  }

  getText(args?: Record<string, unknown>): string {
    let result = this.text;

    // I wasn't able to disable minification in parcel 1.12.3
    // So just let parcel rename ctx arg to e and then:
    result = `const e = ctx;\n` + result;
    result = `const t = ctx;\n` + result;

    for (const name in args) {
      switch (typeof args[name]) {
        case 'number':
          result = `const ${name} = ${args[name]};\n` + result;
          break;
        case 'string':
          result = `const ${name} = '${args[name]}';\n` + result;
          break;
        case 'boolean':
          result = `const ${name} = ${args[name]};\n` + result;
          break;
        case 'object':
          if (Array.isArray(args[name])) {
            result = `const ${name} = [${args[name]}];\n` + result;
          }
          break;
        case 'function':
          result = `const ${name} = ${(args[name] as Function).toString()};\n` + result;
          break;
      }
    }

    return result;
  }

  private getTextWithoutErrorHandling(): string {
    const funcString = this.f.toString().substring(0, this.f.toString().length - 1);
    return funcString.replace(new RegExp('^.+?{', 'm'), '').trim();
  }
}
