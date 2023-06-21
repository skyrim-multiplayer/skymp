export class FunctionInfo<F extends { toString: () => string }> {
  public constructor(private f: F) {}

  get text(): string {
    return (
      `try{${this.getTextWithoutErrorHandling()}}catch(e){` + `ctx.sp.printConsole('[CTX ERROR]', e, '\\n', ${this.f})}`
    );
  }

  getText(args?: Record<string, unknown>): string {
    if (!args) {
      return this.text;
    }
    return `const {${Object.keys(args).join(',')}} = ${JSON.stringify(args)};${this.text}`;
  }

  private getTextWithoutErrorHandling(): string {
    const funcString = this.f.toString().substring(0, this.f.toString().length - 1);
    return funcString.replace(new RegExp('^.+?{', 'm'), '').trim();
  }
}
