import { ClientListener, CombinedController, Sp } from "./clientListener";

export class BlockPapyrusEventsService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        this.controller.once("update", () => this.onceUpdate());
    }

    private onceUpdate() {
        // TODO: It is racing with OnInit in Papyrus, fix it
        (this.sp.TESModPlatform as any).blockPapyrusEvents(true);
    }
};
