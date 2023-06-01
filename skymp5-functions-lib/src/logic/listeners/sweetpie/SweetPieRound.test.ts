import { SweetPieRound, getPlayerCurrentRound, forceJoinRound, forceLeaveRound, determineDeathMatchWinners, getAvailableRound } from "./SweetPieRound";
import { makePlayerController } from "../../TestUtils";

describe("SweetPie", () => {
  test("Is able to find available round", () => {
    const rounds = new Array<SweetPieRound>({ state: 'running', map: { safePointName: 'safepoint', enabled: true } }, { state: 'warmup', map: { safePointName: 'safepoint', enabled: true } });
    expect(getAvailableRound(rounds, 1)).toEqual(rounds[1]);
  });
});

describe("getPlayerCurrentRound", () => {
  test("No rounds present results in undefined", () => {
    const rounds = new Array<SweetPieRound>();
    expect(getPlayerCurrentRound(rounds, 1)).toEqual(undefined);
  });

  test("If player is not in round then return undefined", () => {
    const rounds = new Array<SweetPieRound>({ state: 'warmup' });
    expect(getPlayerCurrentRound(rounds, 1)).toEqual(undefined);
  });

  test("If player is in round then return the round", () => {
    const rounds = new Array<SweetPieRound>({ state: 'warmup' }, { state: 'warmup', players: new Map }, { state: 'warmup', players: new Map([[1, {}]]) });
    expect(getPlayerCurrentRound(rounds, 1)).toEqual(rounds[2]);
  });
});

// TODO: Remove from previous round while joining to a new round
describe("forceJoinRound", () => {
  test("Forcing round join moves player to the safe place of the map", () => {
    const rounds = new Array<SweetPieRound>({ state: 'warmup', map: { safePointName: 'safepoint' } });
    const controller = makePlayerController();
    forceJoinRound(controller, rounds, rounds[0], 1);
    expect(controller.setSpawnPoint).toBeCalledWith(1, 'safepoint');
    expect(controller.teleport).toBeCalledWith(1, 'safepoint');
    expect(rounds[0].players).toBeTruthy();
    expect(rounds[0].players?.has(1)).toBeTruthy();
  });
});

describe("forceLeaveRound", () => {
  test("Leaving non-existing round teleports to the hardcoded hallpoint", () => {
    const rounds = new Array<SweetPieRound>();
    const controller = makePlayerController();
    forceLeaveRound(controller, rounds, 1);
    expect(controller.setSpawnPoint).toBeCalledWith(1, 'hall:spawnPoint');
    expect(controller.teleport).not.toBeCalled();
  });

  test("Leaving a round teleports to the hall and removes from player set", () => {
    const rounds = new Array<SweetPieRound>({ state: 'warmup' }, { state: 'warmup', hallPointName: 'hallpoint', players: new Map([[1, {}]]) });
    const controller = makePlayerController();
    expect(getPlayerCurrentRound(rounds, 1)).toEqual(rounds[1]);
    forceLeaveRound(controller, rounds, 1);
    expect(controller.setSpawnPoint).toBeCalledWith(1, 'hallpoint')
    expect(controller.teleport).toBeCalledWith(1, 'hallpoint');
    expect(getPlayerCurrentRound(rounds, 1)).toEqual(undefined);
  });
});

describe("determineDeathMatchWinners", () => {
  test("Empty round winners are empty array", () => {
    expect(determineDeathMatchWinners({ state: 'warmup' })).toEqual([]);
  });

  test("Should be able to determine single winner", () => {
    const round: SweetPieRound = { state: 'warmup', players: new Map };
    round.players?.set(1, { kills: 20 });
    round.players?.set(2, { kills: 11 });
    expect(determineDeathMatchWinners(round)).toEqual([1]);
  });

  test("Should be able to determine two winners", () => {
    const round: SweetPieRound = { state: 'warmup', players: new Map };
    round.players?.set(1, { kills: 4 });
    round.players?.set(2, { kills: 4 });
    expect(determineDeathMatchWinners(round)).toEqual([1, 2]);
  });

  test("Players can't win with zero kills", () => {
    const round: SweetPieRound = { state: 'warmup', players: new Map };
    round.players?.set(1, { kills: 0 });
    round.players?.set(2, { kills: 0 });
    expect(determineDeathMatchWinners(round)).toEqual([]);
  });
});
