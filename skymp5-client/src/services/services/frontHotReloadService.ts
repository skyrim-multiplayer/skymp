import { AuthGameData, authGameDataStorageKey } from "../../features/authModel";
import { AuthEvent } from "../events/authEvent";
import { ClientListener, Sp, CombinedController } from "./clientListener";

export class FrontHotReloadService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        const enable = !!this.sp.settings["skymp5-client"]["enable-front-hotreload"];

        this.logTrace(`enable-front-hotreload is ${enable}`);

        if (!enable) {
            return;
        }

        // TODO: refactor out very similar code in skympClient.ts
        const authGameData = this.sp.storage[authGameDataStorageKey] as AuthGameData | undefined;

        const storageHasValidAuthGameData = authGameData?.local || authGameData?.remote;

        if (storageHasValidAuthGameData) {
            this.logTrace(`Recovered AuthGameData from storage, starting FrontHotReloadService`);
            this.connectToFrontHotReload();
        } else {
            this.logTrace(`Unable to recover AuthGameData from storage, waiting for auth`);
            this.controller.emitter.on("auth", (e) => this.onAuth(e));
        }
    }

    private onAuth(e: AuthEvent) {
        this.connectToFrontHotReload();
    }

    private connectToFrontHotReload() {
        this.controller.once("update", () => {
            const url = "localhost:1234";
            this.logTrace(`Loading url ${url}`);
            this.sp.browser.loadUrl(url);
        });
    }
}
