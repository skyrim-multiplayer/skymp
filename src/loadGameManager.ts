import * as sp from "skyrimPlatform";

let isCausedBySkyrimPlatform = false;

export interface GameLoadEvent {
    isCausedBySkyrimPlatform: boolean 
};

export type OnGameLoad = (e: GameLoadEvent) => void;

export let addLoadGameListener = (onGameLoad: OnGameLoad) => {
    sp.on('loadGame', () => {
        try {
            onGameLoad({ isCausedBySkyrimPlatform });
        }
        catch(e) {
            sp.once('tick', () => {
                isCausedBySkyrimPlatform = false;
            });
            throw e;
        }
        sp.once('tick', () => {
            isCausedBySkyrimPlatform = false;
        });
    });
};

export let loadGame = (pos: number[], rot: number[], worldOrCell: number) => {
    sp.loadGame(pos, rot, worldOrCell);
    isCausedBySkyrimPlatform = true;
};