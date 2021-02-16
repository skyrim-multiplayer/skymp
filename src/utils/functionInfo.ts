export class FunctionInfo<F extends { toString: () => string }> {
  constructor(private f: F) {}

  get body(): string {
    const funcString = this.f.toString().substring(0, this.f.toString().length - 1);
    return funcString.replace(new RegExp('^.+?{', 'm'), '').trim();
  }
}
