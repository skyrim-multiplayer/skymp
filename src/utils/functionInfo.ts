export class FunctionInfo<F extends { toString: () => string }> {
  constructor(private f: F) {}

  get body(): string {
    const funcString = this.f.toString().substring(0, this.f.toString().length - 1);
    return funcString.replace(new RegExp('^.+?{', 'm'), '').trim();
  }

  tryCatch() {
    return `
      try {
        ${this.body}
      } catch(err) {
        ctx.sp.printConsole('[CTX ERROR]', err, '\\n', ${this.f});
      }`;
  }

  getText(args?: Record<string, unknown>): string {
    let result = this.tryCatch();

    for (let name in args) {
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
}
