import { ClientListener, CombinedController, Sp } from "./clientListener";

export class BlockPapyrusEventsService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        this.controller.once("tick", () => this.onceTick());
    }

    private onceTick() {
        this.sp.blockPapyrusEvents(true);
    }
};
