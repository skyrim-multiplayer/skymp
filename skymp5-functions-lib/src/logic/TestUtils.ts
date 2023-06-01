import { ChatSettings, ServerSettings } from '../types/settings';
import { Counter, PlayerController } from './PlayerController';

export const makePlayerController = (): PlayerController => {
  // TODO: use ES6 Proxy to automate handy methods adding
  const counters = new Map<number, number>();
  return {
    teleport: jest.fn(),
    setSpawnPoint: jest.fn(),
    showMessageBox: jest.fn(),
    sendChatMessage: jest.fn(),
    quitGame: jest.fn(),
    getName: jest.fn((x) => {
      return `Player${x}`;
    }),
    getProfileId: jest.fn(),
    addItem: jest.fn(),
    removeItem: jest.fn(),
    getRoundsArray: jest.fn().mockReturnValue([]),
    setRoundsArray: jest.fn(),
    getOnlinePlayers: jest.fn().mockReturnValue([1, 2]),
    setPercentages: jest.fn(),
    getPercentages: jest.fn(),
    getScriptName: jest.fn(),
    isTeleportActivator: jest.fn().mockReturnValue(true),
    updateCustomName: jest.fn(),
    incrementCounter(actorId: number, counter: Counter, by?: number): number {
      const old = counters.get(actorId) ?? 0;
      counters.set(actorId, old + (by ?? 0));
      return old;
    },
    getActorDistanceSquared(actorId1, actorId2) {
      return 1;
    },
    getServerSetting(name: keyof ServerSettings) {
      const settings: ServerSettings = {
        sweetpieChatSettings: {
          hearingRadiusNormal: 2000,
          whisperDistance: 0.1,
          shoutDistance: 1.5,
          minDistanceToChange: 500,
        },
      };
      return settings[name];
    },
  };
};

export const resetMocks = (controller: PlayerController) => {
  const freshController = makePlayerController();
  for (const key in controller) {
    (controller as Record<string, unknown>)[key] = (freshController as Record<string, unknown>)[key];
  }
};
