
import { DxScanCode } from "@skyrim-platform/skyrim-platform";
import { Sp, CombinedController, ClientListener } from "./clientListener";

export class KeyboardEventsService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        controller.on("update", () => this.onUpdate());
    }

    private onUpdate() {
        const numKeys = this.sp.Input.getNumKeysPressed();

        if (this.lastNumKeys !== numKeys) {
            this.lastNumKeys = numKeys;

            this.controller.emitter.emit("queryKeyCodeBindings", {
                isDown: (binding: DxScanCode[]) => {
                    if (binding.every((key) => this.sp.Input.isKeyPressed(key))) {
                        return true;
                    }
                    return false;
                }
            });
        }
    }

    private lastNumKeys = 0;
};
