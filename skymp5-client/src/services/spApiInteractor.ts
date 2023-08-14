import { EventEmitterFactory } from "./events/eventEmitterFactory";
import { ClientListener, CombinedController } from "./services/clientListener";
import * as sp from "skyrimPlatform";

export class SpApiInteractor {
    static setup(listeners: ClientListener[]) {
    }

    static makeController(): CombinedController {
        return {
            // TODO: handle errors in event handlers. will output to game console by default
            on: sp.on,
            once: sp.once,
            emitter: EventEmitterFactory.makeEventEmitter(),
            lookupListener(listenerName: string): ClientListener {
                const listener = SpApiInteractor.listenersForLookupByName.get(listenerName);
                if (listener === undefined) {
                    throw new Error(`listener not found for name '${listenerName}'`);
                }
                return listener;
            },
        }
    }

    static registerListenerForLookup(listenerName: string, listener: ClientListener): void {
        if (SpApiInteractor.listenersForLookupByName.has(listenerName)) {
            throw new Error(`listener re-registration for name '${listenerName}'`);
        }
        SpApiInteractor.listenersForLookupByName.set(listenerName, listener);
    }

    private static listenersForLookupByName = new Map<string, ClientListener>();
}
