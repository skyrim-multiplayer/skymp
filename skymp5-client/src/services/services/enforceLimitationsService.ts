import { GameLoadEvent } from "../events/gameLoadEvent";
import { ClientListener, CombinedController, Sp } from "./clientListener";

export class EnforceLimitationsService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        controller.once("update", () => this.onceUpdate());
        controller.emitter.on("gameLoad", (e) => this.onGameLoad(e));
    }

    private onceUpdate() {
        this.sp.Game.setInChargen(true, true, false);
    }

    private onGameLoad(event: GameLoadEvent) {
        this.sp.Game.setInChargen(true, true, false);
    }
}
