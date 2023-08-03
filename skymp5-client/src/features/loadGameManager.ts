// import * as sp from "skyrimPlatform";

// let isCausedBySkyrimPlatform = false;

// export interface GameLoadEvent {
//   isCausedBySkyrimPlatform: boolean;
// }

// export type OnGameLoad = (e: GameLoadEvent) => void;

// export const addLoadGameListener = (onGameLoad: OnGameLoad): void => {
//   sp.on("loadGame", () => {
//     try {
//       onGameLoad({ isCausedBySkyrimPlatform });
//     } catch (e) {
//       sp.once("tick", () => {
//         isCausedBySkyrimPlatform = false;
//       });
//       throw e;
//     }
//     sp.once("tick", () => {
//       isCausedBySkyrimPlatform = false;
//     });
//   });
// };

// export const loadGame = (
//   pos: number[],
//   rot: number[],
//   worldOrCell: number,
//   changeFormNpc?: sp.ChangeFormNpc
// ): void => {
//   sp.loadGame(pos, rot, worldOrCell, changeFormNpc);
//   isCausedBySkyrimPlatform = true;
// };
