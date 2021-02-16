import * as fs from 'fs';

const getFileContents = (path: string): string => {
  if (fs.existsSync(path)) {
    return fs.readFileSync(path, { encoding: 'utf-8' });
  }
  return '';
};

export class LocalizationProvider {
  constructor(private localizationFilePath: string, private mode: 'hotreload' | 'once') {
    this.localization = {};
  }

  getText(msgId: string): string {
    if (Object.keys(this.localization).length === 0) {
      const contents = getFileContents(this.localizationFilePath);
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
