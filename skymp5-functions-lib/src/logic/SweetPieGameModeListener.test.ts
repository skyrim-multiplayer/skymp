import { sprintf } from "../lib/sprintf-js";
import { SweetPieGameModeListener } from "./SweetPieGameModeListener";
import { SweetPieMap } from "./SweetPieMap";
import { getPlayerCurrentRound, forceJoinRound, forceLeaveRound, determineDeathMatchWinners } from "./SweetPieRound";
import { makePlayerController, resetMocks } from "./TestUtils";

describe("SweetPieGameModeListener: Activation default", () => {
  test("Activators should continue by default", () => {
    const controller = makePlayerController();
    const listener = new SweetPieGameModeListener(controller);

    const res = [
      listener.onPlayerActivateObject(1, "2beef", false),
      listener.onPlayerActivateObject(1, "1beef", true)
    ];
    expect(res).toEqual(['continue', 'continue']);
  });
});

describe("SweetPieGameModeListener: Portals", () => {
  test("Quit game portal should show quit game dialog", () => {
    const controller = makePlayerController();
    const listener = new SweetPieGameModeListener(controller);
    const res = listener.onPlayerActivateObject(1, listener.quitGamePortal, false);
    expect(controller.showMessageBox).toBeCalledWith(1, ...listener.quitDialog);
    expect(res).toEqual('blockActivation');

    resetMocks(controller);
    listener.onPlayerDialogResponse(1, listener.quitDialog[0], 0);
    expect(controller.quitGame).toBeCalledWith(1);
  });

  test("Neutral portal should show deathmatch dialog", () => {
    const controller = makePlayerController();
    const listener = new SweetPieGameModeListener(controller);
    const res = listener.onPlayerActivateObject(1, listener.neutralPortal, false);
    expect(controller.showMessageBox).toBeCalledWith(1, ...listener.joinDeathMatchDialog);
    expect(res).toEqual('blockActivation');
  });
});

describe("SweetPieGameModeListener: DeathMatch", () => {
  test("Player should be able to join round via dialog window", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace' }];
    const listener = new SweetPieGameModeListener(controller, maps);

    // We click No
    listener.onPlayerDialogResponse(1, listener.joinDeathMatchDialog[0], 1);
    expect(controller.teleport).toBeCalledTimes(0);
    expect(controller.setSpawnPoint).toBeCalledTimes(0);
    expect(getPlayerCurrentRound(listener.getRounds(), 1)).toEqual(undefined);

    // We click Yes and teleport to the safe place of the round's map
    // It's usually a tavern
    listener.onPlayerDialogResponse(1, listener.joinDeathMatchDialog[0], 0);
    expect(controller.teleport).toBeCalledWith(1, 'whiterun:safePlace');
    expect(controller.setSpawnPoint).toBeCalledWith(1, 'whiterun:safePlace');
    expect(getPlayerCurrentRound(listener.getRounds(), 1)).toBeTruthy();
  });

  test("Player should be able to leave the safe place", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace', safePlaceLeaveDoors: ['bbb'] }];
    const listener = new SweetPieGameModeListener(controller, maps);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);

    // Activate one of safePlaceLeaveDoors and leave
    // Players fight outside of safe place
    // Do not check that door teleports the player since teleport doors do this by default
    expect(listener.onPlayerActivateObject(1, 'bbb', true)).toEqual('continue');
  });

  test("Player should not be able to go back to the safe place", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace', safePlaceEnterDoors: ['bbb'] }];
    const listener = new SweetPieGameModeListener(controller, maps);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);

    const res = listener.onPlayerActivateObject(1, 'bbb', true);
    expect(controller.showMessageBox).toBeCalledWith(1, ...listener.noEnterSafePlaceDialog);
    expect(res).toEqual('blockActivation');
  });

  test("Player should be able to leave the round by using city door", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace', leaveRoundDoors: ['bbb'] }];
    const listener = new SweetPieGameModeListener(controller, maps);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);

    // Players press E on city gates and see the dialog
    const res = listener.onPlayerActivateObject(1, 'bbb', true);
    expect(controller.showMessageBox).toBeCalledWith(1, ...listener.leaveRoundConfirmationDialog);
    expect(res).toEqual('blockActivation');

    // Player clicks No. Nothing happens
    resetMocks(controller);
    listener.onPlayerDialogResponse(1, listener.leaveRoundConfirmationDialog[0], 1);
    expect(controller.teleport).toBeCalledTimes(0);

    listener.getRounds()[0].state = 'running';

    // Player clicks Yes. Now it was removed from the round
    listener.onPlayerDialogResponse(1, listener.leaveRoundConfirmationDialog[0], 0);
    expect(controller.teleport).toBeCalledWith(1, 'hall:spawnPoint');
    expect(controller.setSpawnPoint).toBeCalledWith(1, 'hall:spawnPoint');
    expect(getPlayerCurrentRound(listener.getRounds(), 1)).toEqual(undefined);
    expect(listener.getRounds()[0].state).toEqual('warmup');

    // Teleport even if the player isn't in any round
    resetMocks(controller);
    listener.onPlayerDialogResponse(1, listener.leaveRoundConfirmationDialog[0], 0);
    expect(controller.teleport).toBeCalledWith(1, 'hall:spawnPoint');
    expect(controller.setSpawnPoint).toBeCalledWith(1, 'hall:spawnPoint');
  });

  test("Player attempts to hide from fight in interior", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace' }];
    const listener = new SweetPieGameModeListener(controller, maps);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);

    const res = listener.onPlayerActivateObject(1, "beb", true);
    expect(controller.sendChatMessage).toBeCalledWith(1, ...listener.interiorsBlockedMessage);
    expect(res).toEqual('blockActivation');
  });
});

describe("SweetPieGameModeListener: Chat", () => {
  test("Chat messages are transferred to neighbors", () => {
    const controller = makePlayerController();
    const listener = new SweetPieGameModeListener(controller);
    listener.onPlayerChatInput(1, "hello!", [1, 2, 3], "SupAidme");

    const msg = '#{a8adad}' + 'SupAidme' + '#{ffffff}: ' + 'hello!';
    expect(controller.sendChatMessage).toBeCalledTimes(3);
    expect(controller.sendChatMessage).toHaveBeenCalledWith(1, msg);
    expect(controller.sendChatMessage).toHaveBeenCalledWith(2, msg);
    expect(controller.sendChatMessage).toHaveBeenCalledWith(3, msg);
  });
});

describe("SweetPieGameModeListener: OnJoin", () => {
  test("SpawnPoint must be set to hall for joined players", () => {
    const controller = makePlayerController();
    const listener = new SweetPieGameModeListener(controller);

    listener.onPlayerJoin(1);
    expect(controller.setSpawnPoint).toBeCalledWith(1, 'hall:spawnPoint');
  });
});

describe("SweetPieGameModeListener: Round clock", () => {
  test("Round must tick only if players present", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace' }];
    const listener = new SweetPieGameModeListener(controller, maps);

    expect(listener.getRounds()[0].secondsPassed).toBe(0);
    listener.everySecond();
    expect(listener.getRounds()[0].secondsPassed).toBe(0);

    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);
    expect(listener.getRounds()[0].secondsPassed).toBe(0);

    listener.everySecond();
    expect(listener.getRounds()[0].secondsPassed).toBe(1);

    // Pause warmup when players leave the round
    forceLeaveRound(controller, listener.getRounds(), 1);
    expect(listener.getRounds()[0].secondsPassed).toBe(1);
  });

  // TODO: Do not start if there are not enough players
  // TODO: Start right now if there are maximum players
  test("Round warmup must finish once timer reaches maximum", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace', mainSpawnPointName: 'whiterun:spawnPoint' }];
    const listener = new SweetPieGameModeListener(controller, maps);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);
    resetMocks(controller);
    listener.getRounds()[0].secondsPassed = listener.warmupTimerMaximum;

    listener.everySecond();
    expect(listener.getRounds()[0].secondsPassed).toBe(0);
    expect(listener.getRounds()[0].state).toBe('running');

    // 'You have 300 seconds to fight'
    const expectedMsg = sprintf(listener.warmupFinishedMessage[0], listener.runningTimerMaximum);
    expect(controller.sendChatMessage).toBeCalledWith(1, expectedMsg);

    expect(controller.teleport).toBeCalledWith(1, 'whiterun:spawnPoint');
    expect(controller.setSpawnPoint).toBeCalledWith(1, 'whiterun:spawnPoint');
  });

  test("Round warmup must output messages about remaining seconds", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace' }];
    const listener = new SweetPieGameModeListener(controller, maps);
    listener.warmupTimerMaximum = 30;

    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);

    for (let i = 0; i < 30; i++) {
      listener.everySecond();
    }

    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 20 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 10 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 9 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 8 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 7 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 6 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 5 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 4 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 3 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 2 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Starting round in 1 seconds");
  });

  test("Fight must finish once timer reaches maximum", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace' }];
    const listener = new SweetPieGameModeListener(controller, maps);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);
    resetMocks(controller);
    listener.getRounds()[0].secondsPassed = listener.runningTimerMaximum;
    listener.getRounds()[0].state = 'running';

    listener.everySecond();

    expect(listener.getRounds()[0].secondsPassed).toBe(0);
    expect(listener.getRounds()[0].players).toBe(undefined);
    expect(listener.getRounds()[0].state).toBe('warmup');
    expect(controller.teleport).toBeCalledWith(1, 'hall:spawnPoint');
    expect(controller.setSpawnPoint).toBeCalledWith(1, 'hall:spawnPoint');
  });

  test("Round must finish without winner if there is no winner", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace' }];
    const listener = new SweetPieGameModeListener(controller, maps);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);
    resetMocks(controller);
    listener.getRounds()[0].secondsPassed = listener.runningTimerMaximum;
    listener.getRounds()[0].state = 'running';

    listener.everySecond();

    // 'No one wins'
    const expectedMsg = sprintf(listener.warmupFinishedMessage[0], listener.runningTimerMaximum);
    expect(controller.sendChatMessage).toBeCalledWith(1, ...listener.noWinnerMessage);
  });

  test("Round must finish with single winner if there is top player", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace' }];
    const listener = new SweetPieGameModeListener(controller, maps);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 2);
    resetMocks(controller);
    listener.getRounds()[0].secondsPassed = listener.runningTimerMaximum;
    listener.getRounds()[0].state = 'running';
    listener.getRounds()[0].players?.set(1, { kills: 10 });
    listener.getRounds()[0].players?.set(2, { kills: 3 });

    listener.everySecond();

    // 'Player1 wins with 10 points'
    const msg = sprintf(listener.determineWinnerMessage[0], controller.getName(1), 10);
    expect(controller.sendChatMessage).toBeCalledTimes(2);
    expect(controller.sendChatMessage).toBeCalledWith(1, msg);
    expect(controller.sendChatMessage).toBeCalledWith(2, msg);
  });

  test("Round must finish with two winner if both players are tops", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace' }];
    const listener = new SweetPieGameModeListener(controller, maps);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 2);
    resetMocks(controller);
    listener.getRounds()[0].secondsPassed = listener.runningTimerMaximum;
    listener.getRounds()[0].state = 'running';
    listener.getRounds()[0].players?.set(1, { kills: 3 });
    listener.getRounds()[0].players?.set(2, { kills: 3 });

    listener.everySecond();

    // 'We have multiple winners!'
    // 'Player1 wins with 3 points'
    // 'Player2 wins with 3 points'
    const msg1 = sprintf(listener.determineWinnerMessage[0], controller.getName(1), 3);
    const msg2 = sprintf(listener.determineWinnerMessage[0], controller.getName(2), 3);
    expect(controller.sendChatMessage).toBeCalledTimes(6);
    expect(controller.sendChatMessage).toBeCalledWith(1, ...listener.multipleWinnersMessage);
    expect(controller.sendChatMessage).toBeCalledWith(1, msg1);
    expect(controller.sendChatMessage).toBeCalledWith(1, msg2);
    expect(controller.sendChatMessage).toBeCalledWith(2, ...listener.multipleWinnersMessage);
    expect(controller.sendChatMessage).toBeCalledWith(2, msg1);
    expect(controller.sendChatMessage).toBeCalledWith(2, msg2);
  });

  test("Round fight must output messages about remaining seconds", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace' }];
    const listener = new SweetPieGameModeListener(controller, maps);
    listener.runningTimerMaximum = 30;
    listener.getRounds()[0].state = 'running';

    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);

    for (let i = 0; i < 30; i++) {
      listener.everySecond();
    }

    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 20 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 10 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 9 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 8 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 7 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 6 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 5 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 4 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 3 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 2 seconds");
    expect(controller.sendChatMessage).toBeCalledWith(1, "Fight! You have 1 seconds");
  });
});

describe("SweetPieGameModeListener: OnDeath", () => {
  it("Gives score to killer", () => {
    const controller = makePlayerController();
    const maps: SweetPieMap[] = [{ safePointName: 'whiterun:safePlace' }];
    const listener = new SweetPieGameModeListener(controller, maps);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 1);
    forceJoinRound(controller, listener.getRounds(), listener.getRounds()[0], 2);
    resetMocks(controller);

    listener.getRounds()[0].state = 'warmup';
    listener.onPlayerDeath(1, 2);

    expect(controller.sendChatMessage).not.toBeCalled();

    listener.getRounds()[0].state = 'running';
    listener.onPlayerDeath(1, 2);

    // %s was slain by %s. %s now has %d points (the best is %d)
    const msg = sprintf(listener.deathMessage[0], controller.getName(1), controller.getName(2), controller.getName(2), 1, 1);
    expect(controller.sendChatMessage).toBeCalledTimes(2);
    expect(controller.sendChatMessage).toBeCalledWith(1, msg);
    expect(controller.sendChatMessage).toBeCalledWith(2, msg);

    expect(determineDeathMatchWinners(listener.getRounds()[0])).toEqual([2]);
  });
});
