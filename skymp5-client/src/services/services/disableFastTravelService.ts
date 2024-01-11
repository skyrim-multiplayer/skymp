import { ClientListener, Sp, CombinedController } from "./clientListener";

export class DisableFastTravelService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.on("update", () => this.onUpdate());
    }

    private onUpdate() {
        this.sp.Game.enableFastTravel(false);
    }
}
