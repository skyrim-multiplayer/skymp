export class FunctionInfo<F extends { toString: () => string }> {
  public constructor(private f: F) {}

  get text(): string {
    // this part is different to functionInfo.ts used in skymp5-functions-lib
    return this.getTextWithoutErrorHandling();
  }

  getText(args?: Record<string, unknown>): string {
    if (!args) {
      return this.text;
    }
    return `(function(){const {${Object.keys(args).join(',')}} = ${JSON.stringify(args)};${this.text}})()`;
  }

  private getTextWithoutErrorHandling(): string {
    const funcString = this.f.toString().substring(0, this.f.toString().length - 1);
    return funcString.replace(new RegExp('^.+?{', 'm'), '').trim();
  }
}
