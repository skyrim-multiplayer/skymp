import { Mp } from '../types/mp';

export class LocalizationProvider {
  constructor(private mp: Mp, private localizationFilePath: string, private mode: 'hotreload' | 'once') {
    this.localization = {};
  }

  getText(msgId: string): string {
    if (Object.keys(this.localization).length === 0) {
      const contents = this.mp.readDataFile(this.localizationFilePath);
      try {
        this.localization = JSON.parse(contents);
      } catch (e) {
        this.localization = {};
      }
    }
    const res = this.localization[msgId] || msgId;
    if (this.mode === 'hotreload') {
      this.localization = {};
    }
    return res;
  }

  private localization: Record<string, string | undefined>;
}
