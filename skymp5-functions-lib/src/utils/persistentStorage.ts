import { SweetPieRound } from "../logic/listeners/sweetpie/SweetPieRound";
import { Mp, LocationalData } from "../types/mp";

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
    const roundsData = mp.get(storeObject, "private.rounds") || [];
    if (Array.isArray(roundsData) && !roundsData.find((x) => typeof x !== 'object')) {
      const rounds: SweetPieRound[] = [];
      for (const round of roundsData) {
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
    mp.set(storeObject, "private.rounds", rounds);
  }

  get reloads(): number {
    return mp.get(storeObject, "private.reloads") || 0;
  }

  set reloads(newValue: number) {
    mp.set(storeObject, "private.reloads", newValue);
  }

  get onlinePlayers(): number[] {
    const onlinePlayersData = mp.get(storeObject, "private.onlinePlayers") || [];
    if (Array.isArray(onlinePlayersData)) {
      if (onlinePlayersData.filter((x) => typeof x === 'number').length === onlinePlayersData.length) {
        return onlinePlayersData;
      }
    }
    return [];
  }

  set onlinePlayers(newValue: number[]) {
    mp.set(storeObject, "private.onlinePlayers", newValue);
  }

  get teleports(): TeleportData {
    return mp.get(storeObject, "private.teleports") || {};
  }

  set teleports(newValue: TeleportData) {
    mp.set(storeObject, "private.teleports", newValue);
  }

  private constructor() {}

  private static instance: PersistentStorage;
}
