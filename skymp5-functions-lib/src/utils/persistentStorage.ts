import { LocationalData, Mp } from '../types/mp';
import { SweetPieRound } from "../logic/listeners/sweetpie/SweetPieRound";

declare const mp: Mp;

// A world object: HonningBrew Boilery entrance.
// Used to store data as private vars
const storeObject = 0x7d7a8;

interface TeleportData {
  [key: string]: LocationalData;
}

export class PersistentStorage {
  static getSingleton(): PersistentStorage {
    if (!PersistentStorage.instance) {
      PersistentStorage.instance = new PersistentStorage();
    }
    return PersistentStorage.instance;
  }

  get rounds(): SweetPieRound[] {
    const obj = mp.get(storeObject, "private.persistentStorage") || {};
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
    const obj = mp.get(storeObject, "private.persistentStorage") || {};
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
    mp.set(storeObject, "private.persistentStorage", obj);
  }

  get reloads(): number {
    const obj = mp.get(storeObject, "private.persistentStorage") || {};
    return typeof obj['reloads'] === 'number' ? obj['reloads'] : 0;
  }

  set reloads(newValue: number) {
    const obj = mp.get(storeObject, "private.persistentStorage") || {};
    obj['reloads'] = newValue;
    mp.set(storeObject, "private.persistentStorage", obj);
  }

  get onlinePlayers(): number[] {
    const obj = mp.get(storeObject, "private.persistentStorage") || {};
    if (Array.isArray(obj['onlinePlayers'])) {
      if (obj['onlinePlayers'].filter((x) => typeof x === 'number').length === obj['onlinePlayers'].length) {
        return obj['onlinePlayers'];
      }
    }
    return [];
  }

  set onlinePlayers(newValue: number[]) {
    const obj = mp.get(storeObject, "private.persistentStorage") || {};
    obj['onlinePlayers'] = newValue;
    mp.set(storeObject, "private.persistentStorage", obj);
  }

  get teleports(): TeleportData {
    const obj = mp.get(storeObject, "private.persistentStorage") || {};
    if (typeof obj['teleports'] === "object") {
      return obj['teleports'];
    }
    return {};
  }

  set teleports(newValue: TeleportData) {
    const obj = mp.get(storeObject, "private.persistentStorage") || {};
    obj['teleports'] = newValue;
    mp.set(storeObject, "private.persistentStorage", obj);
  }

  private constructor() {}

  private static instance: PersistentStorage;
}
