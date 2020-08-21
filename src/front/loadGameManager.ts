import { on, once, loadGame as spLoadGame } from "@skymp/skyrim-platform";

let isCausedBySkyrimPlatform = false;

export interface GameLoadEvent {
  isCausedBySkyrimPlatform: boolean;
}

export type OnGameLoad = (e: GameLoadEvent) => void;

export const addLoadGameListener = (onGameLoad: OnGameLoad): void => {
  on("loadGame", () => {
    try {
      onGameLoad({ isCausedBySkyrimPlatform });
    } catch (e) {
      once("tick", () => {
        isCausedBySkyrimPlatform = false;
      });
      throw e;
    }
    once("tick", () => {
      isCausedBySkyrimPlatform = false;
    });
  });
};

export const loadGame = (
  pos: number[],
  rot: number[],
  worldOrCell: number
): void => {
  spLoadGame(pos, rot, worldOrCell);
  isCausedBySkyrimPlatform = true;
};
