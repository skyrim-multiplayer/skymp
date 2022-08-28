import { Mp } from '../types/mp';

declare const mp: Mp;

export class PersistentStorage {
  static getSingleton(): PersistentStorage {
    if (!PersistentStorage.instance) {
      PersistentStorage.instance = new PersistentStorage();
    }
    return PersistentStorage.instance;
  }

  get reloads(): number {
    const str = mp.readDataFile('persistentStorage.json');
    const obj = this.parse(str);
    return typeof obj['reloads'] === 'number' ? obj['reloads'] : 0;
  }

  set reloads(newValue: number) {
    const str = mp.readDataFile('persistentStorage.json');
    const obj = this.parse(str);
    obj['reloads'] = newValue;
    mp.writeDataFile('persistentStorage.json', JSON.stringify(obj, null, 2));
  }

  get onlinePlayers(): number[] {
    const str = mp.readDataFile('persistentStorage.json');
    const obj = this.parse(str);
    if (Array.isArray(obj['onlinePlayers'])) {
      if (obj['onlinePlayers'].filter((x) => typeof x === 'number').length === obj['onlinePlayers'].length) {
        return obj['onlinePlayers'];
      }
    }
    return [];
  }

  set onlinePlayers(newValue: number[]) {
    const str = mp.readDataFile('persistentStorage.json');
    const obj = this.parse(str);
    obj['onlinePlayers'] = newValue;
    mp.writeDataFile('persistentStorage.json', JSON.stringify(obj, null, 2));
  }

  private constructor() {}

  private parse(str: string): Record<string, unknown> {
    if (str === '') {
      return {};
    }
    return JSON.parse(str);
  }

  private static instance: PersistentStorage;
}
