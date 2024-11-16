import { EventEmitterFactory } from "./events/events";
import { ClientListener, ClientListenerConstructor, CombinedController } from "./services/clientListener";
import * as sp from "skyrimPlatform";

export class SpApiInteractor {
    static setup(listeners: ClientListener[]) {
        listeners.forEach(listener => SpApiInteractor.registerListenerForLookup(listener.constructor, listener));
    }

    static getControllerInstance(): CombinedController {
        if (SpApiInteractor.controller) {
            return SpApiInteractor.controller;
        }
        SpApiInteractor.controller = {
            // TODO: handle errors in event handlers. will output to game console by default
            on: sp.on,
            once: sp.once,
            emitter: EventEmitterFactory.makeEventEmitter(),
            lookupListener<T extends ClientListener>(constructor: ClientListenerConstructor<T>): T {
                const listener = SpApiInteractor.listenersForLookupByName.get(constructor);
                if (listener === undefined) {
                    throw new Error(`listener not found for name '${onsctructor.name}'`);
                }
                if (!(listener instanceof constructor)) {
                    throw new Error(`listener class mismatch for name '${constructor.name}'`);
                }
                return listener;
            },
        }
        return SpApiInteractor.controller;
    }

    private static registerListenerForLookup(constructor: Function, listener: ClientListener): void {
        if (SpApiInteractor.listenersForLookupByName.has(constructor)) {
            throw new Error(`listener re-registration for name '${constructor}'`);
        }
        SpApiInteractor.listenersForLookupByName.set(constructor, listener);
    }

    private static listenersForLookupByName = new Map<Function, ClientListener>();

    private static controller?: CombinedController;
}
