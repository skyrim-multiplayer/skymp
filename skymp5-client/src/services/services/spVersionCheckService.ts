import { requiredVersion } from "../../version";
import { ClientListener, Sp, CombinedController } from "./clientListener";

export class SpVersionCheckService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        controller.once("update", () => this.onceUpdate());
    }

    private onceUpdate() {
        const realVersion = this.sp.getPlatformVersion();

        if (!requiredVersion.includes(realVersion)) {
            this.sp.Debug.messageBox(
                `You need to have on of those SkyrimPlatform versions ${JSON.stringify(
                    requiredVersion,
                )} to join this server. Your current version is ${realVersion}`,
            );
            this.sp.Utility.waitMenuMode(0.5).then(() => {
                this.controller.on('update', () => {
                    if (!this.sp.Ui.isMenuOpen('MessageBoxMenu')) {
                        this.sp.Game.quitToMainMenu();
                    }
                });
            });
        }
    }
}
