import { BlockPapyrusEventsService } from "./blockPapyrusEventsService";
import { ClientListener, CombinedController } from "./clientListener";
import * as sp from "skyrimPlatform";
import { LoadGameService } from "./loadGameService";
import { SinglePlayerService } from "./singlePlayerService";
import { EnforceLimitationsService } from "./enforceLimitationsService";
import { SendInputsService } from "./sendInputsService";

export class SpApiInteractor {
    static setup(listeners: ClientListener[]) {
    }

    static makeController(): CombinedController {
        return {
            // TODO: handle errors in event handlers. will output to game console by default
            on: sp.on,
            once: sp.once,
            registerListenerForLookup(listenerName: string, listener: ClientListener): void {
                if (SpApiInteractor.listenersForLookupByName.has(listenerName)) {
                    throw new Error(`listener re-registration for name '${listenerName}'`);
                }
                SpApiInteractor.listenersForLookupByName.set(listenerName, listener);
            },
            lookupListener(listenerName: string): ClientListener {
                const listener = SpApiInteractor.listenersForLookupByName.get(listenerName);
                if (listener === undefined) {
                    throw new Error(`listener not found for name '${listenerName}'`);
                }
                return listener;
            },
        }
    }

    private static listenersForLookupByName = new Map<string, ClientListener>();
}
