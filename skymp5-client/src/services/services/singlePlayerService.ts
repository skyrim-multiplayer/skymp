import { ClientListener, CombinedController, Sp } from "./clientListener";
import * as networking from "../../networking";
import { GameLoadEvent } from "../events/gameLoadEvent";

export class SinglePlayerService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        this.controller.registerListenerForLookup("SinglePlayerService", this);
        this.controller.emitter.on("gameLoad", (e) => this.onGameLoad(e));
    }

    get isSinglePlayer() {
        return this._isSinglePlayer;
    }

    private onGameLoad(event: GameLoadEvent) {
        if (!event.isCausedBySkyrimPlatform && !this._isSinglePlayer) {
            this.sp.Debug.messageBox(
                'Save has been loaded in multiplayer, switching to the single-player mode',
            );
            networking.close();
            this._isSinglePlayer = true;
            this.sp.Game.setInChargen(false, false, false);
        }
    }

    private _isSinglePlayer = false;
}
