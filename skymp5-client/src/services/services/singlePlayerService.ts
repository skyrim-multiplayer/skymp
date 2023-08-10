import { ClientListener, CombinedController, Sp } from "./clientListener";
import { GameLoadEvent, LoadGameService } from "./loadGameService";
import * as networking from "../../networking";

export class SinglePlayerService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        this.controller.registerListenerForLookup("SinglePlayerService", this);

        const loadGameService = this.controller.lookupListener("LoadGameService") as LoadGameService;
        loadGameService.events.on("gameLoad", (e) => this.onGameLoad(e));
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
