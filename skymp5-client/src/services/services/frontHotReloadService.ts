import { logTrace } from "../../logging";
import { AuthGameData, authGameDataStorageKey } from "../../features/authModel";
import { AuthAttemptEvent } from "../events/authAttemptEvent";
import { ClientListener, Sp, CombinedController } from "./clientListener";

export class FrontHotReloadService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        const enable = !!this.sp.settings["skymp5-client"]["enable-front-hotreload"];

        logTrace(this, `enable-front-hotreload is`, enable);

        if (!enable) {
            return;
        }

        // TODO: refactor out very similar code in skympClient.ts
        const authGameData = this.sp.storage[authGameDataStorageKey] as AuthGameData | undefined;

        const storageHasValidAuthGameData = authGameData?.local || authGameData?.remote;

        if (storageHasValidAuthGameData) {
            logTrace(this, `Recovered AuthGameData from storage, starting FrontHotReloadService`);
            this.connectToFrontHotReload();
        } else {
            logTrace(this, `Unable to recover AuthGameData from storage, waiting for auth`);
            this.controller.emitter.on("authAttempt", (e) => this.onAuthAttempt(e));
        }
    }

    private onAuthAttempt(e: AuthAttemptEvent) {
        this.connectToFrontHotReload();
    }

    private connectToFrontHotReload() {
        this.controller.once("update", () => {
            const url = "localhost:1234";
            logTrace(this, `Loading url`, url);
            this.sp.browser.loadUrl(url);
        });
    }
}
