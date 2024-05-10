import { GamemodeApiCtx } from "./gamemodeApiCtx";

export interface GamemodeApiEventSourceCtx extends GamemodeApiCtx {
    sendEvent: (...args: unknown[]) => void;
    _fn: Function;
    _eventName: string;
}
