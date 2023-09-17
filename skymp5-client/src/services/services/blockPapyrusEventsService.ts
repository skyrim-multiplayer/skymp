import { ClientListener, CombinedController, Sp } from "./clientListener";

export class BlockPapyrusEventsService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        this.controller.once("tick", () => this.onceTick());
        this.controller.once("update", () => this.onceUpdate());
    }

    private onceTick() {
        if (typeof this.sp.blockPapyrusEvents === "function") {
            this.sp.blockPapyrusEvents(true);
        }
    }

    private onceUpdate() {
        (this.sp.TESModPlatform as any).blockPapyrusEvents(true);
    }
};
