import { Mp } from '../types/mp';
import { SweetPieRound } from "../logic/SweetPieRound";

declare const mp: Mp;

export class PersistentStorage {
  static getSingleton(): PersistentStorage {
    if (!PersistentStorage.instance) {
      PersistentStorage.instance = new PersistentStorage();
    }
    return PersistentStorage.instance;
  }

  get rounds(): SweetPieRound[] {
    const str = mp.readDataFile('persistentStorage.json');
    const obj = this.parse(str);
    if (Array.isArray(obj['rounds']) && !obj['rounds'].find((x) => typeof x !== 'object')) {
      const rounds: SweetPieRound[] = [];
      for (const round of obj['rounds']) {
        const players = new Map<number, { kills?: number }>();
        for (const obj of round['players']) {
          players.set(
            Number.parseInt(obj['key']),
            typeof obj['val'] === 'object' ? obj['val'] : {}
          );
        }
        rounds.push({
          state: round['state'],
          map: round['map'],
          hallPointName: round['hallPointName'],
          secondsPassed: round['secondsPassed'],
          players: players
        });
      }
      return rounds;
    }
    return [];
  }

  set rounds(newValue: SweetPieRound[]) {
    const str = mp.readDataFile('persistentStorage.json');
    const obj = this.parse(str);
    const rounds = new Array();
    for (const round of newValue) {
      const players = new Array();
      for (const [key, val] of round.players || new Map()) {
        players.push({ key: key, val: val });
      }
      rounds.push({
        state: round.state,
        map: round.map,
        hallPointName: round.hallPointName,
        secondsPassed: round.secondsPassed,
        players: players
      });
    }
    obj['rounds'] = rounds;
    mp.writeDataFile('persistentStorage.json', JSON.stringify(obj, null, 2));
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
