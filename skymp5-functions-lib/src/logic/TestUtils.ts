import { PlayerController } from "./PlayerController";

export const makePlayerController = (): PlayerController => {
  // TODO: use ES6 Proxy to automate handy methods adding
  // TODO: fix getName always returning undefined
  return {
    teleport: jest.fn(), setSpawnPoint: jest.fn(), showMessageBox: jest.fn(), sendChatMessage: jest.fn(), quitGame: jest.fn(), getName: jest.fn(x => { return `Player${x}`; })
  };
};

export const resetMocks = (controller: PlayerController) => {
  for (const key in controller) {
    (controller as Record<string, unknown>)[key] = jest.fn();
  }
}
