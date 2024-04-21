import { ClientListener, CombinedController, Sp } from "./clientListener";

export class BlockPapyrusEventsService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.once("tick", () => this.onceTick());
        this.controller.once("update", () => this.onceUpdate());
    }

    private onceTick() {
        // @ts-ignore
        this.sp.blockPapyrusEvents(true);
    }

    private onceUpdate() {
        (this.sp.TESModPlatform as any).blockPapyrusEvents(true);
    }
};
