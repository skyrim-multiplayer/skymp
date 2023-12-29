import { EventEmitterFactory } from "./events/events";
import { ClientListener, ClientListenerConstructor, CombinedController } from "./services/clientListener";
import * as sp from "skyrimPlatform";

export class SpApiInteractor {
    static setup(listeners: ClientListener[]) {
    }

    static makeController(): CombinedController {
        if (SpApiInteractor.controller) {
            return SpApiInteractor.controller;
        }
        SpApiInteractor.controller = {
            // TODO: handle errors in event handlers. will output to game console by default
            on: sp.on,
            once: sp.once,
            emitter: EventEmitterFactory.makeEventEmitter(),
            lookupListener<T extends ClientListener>(constructor: ClientListenerConstructor<T>): T {
                const listener = SpApiInteractor.listenersForLookupByName.get(constructor.name);
                if (listener === undefined) {
                    throw new Error(`listener not found for name '${constructor.name}'`);
                }
                if (!(listener instanceof constructor)) {
                    throw new Error(`listener class mismatch for name '${constructor.name}'`);
                }
                return listener;
            },
        }
        return SpApiInteractor.controller;
    }

    static registerListenerForLookup(listenerName: string, listener: ClientListener): void {
        if (SpApiInteractor.listenersForLookupByName.has(listenerName)) {
            throw new Error(`listener re-registration for name '${listenerName}'`);
        }
        SpApiInteractor.listenersForLookupByName.set(listenerName, listener);
    }

    private static listenersForLookupByName = new Map<string, ClientListener>();

    private static controller?: CombinedController;
}
