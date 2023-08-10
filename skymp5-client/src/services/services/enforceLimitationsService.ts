import { ClientListener, CombinedController, Sp } from "./clientListener";
import { GameLoadEvent, LoadGameService } from "./loadGameService";

export class EnforceLimitationsService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        controller.once("update", () => this.onceUpdate());

        const loadGameService = this.controller.lookupListener("LoadGameService") as LoadGameService;
        loadGameService.events.on("gameLoad", (e) => this.onGameLoad(e));
    }

    private onceUpdate() {
        this.sp.Game.setInChargen(true, true, false);
    }

    private onGameLoad(event: GameLoadEvent) {
        this.sp.Game.setInChargen(true, true, false);
    }
}
