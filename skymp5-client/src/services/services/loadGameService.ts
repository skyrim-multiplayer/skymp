import { EventEmitter } from "eventemitter3";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { ChangeFormNpc } from "skyrimPlatform";
import { GameLoadEvent } from "../events/gameLoadEvent";

export class LoadGameService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        this.controller.registerListenerForLookup("LoadGameService", this);
        this.controller.on("loadGame", () => this.onLoadGame());
    }

    public loadGame(pos: number[], rot: number[], worldOrCell: number, changeFormNpc?: ChangeFormNpc) {
        this.sp.loadGame(pos, rot, worldOrCell, changeFormNpc);
        this._isCausedBySkyrimPlatform = true;
    }

    private onLoadGame() {
        try {
            const gameLoadEvent = {
                isCausedBySkyrimPlatform: this._isCausedBySkyrimPlatform
            };
            this.controller.emitter.emit("gameLoad", gameLoadEvent);
        } catch (e) {
            this.controller.once("tick", () => {
                this._isCausedBySkyrimPlatform = false;
            });
            throw e;
        }
        this.controller.once("tick", () => {
            this._isCausedBySkyrimPlatform = false;
        });
    }

    private _isCausedBySkyrimPlatform = false;
}
