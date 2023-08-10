import * as sp from "skyrimPlatform";
import { EventEmitterType } from "../events/eventEmitterFactory";

export interface ClientListenerEvents { 
    on: typeof sp.on, 
    once: typeof sp.once 
};

export type Sp = Omit<typeof sp, "on" | "once">;

export interface ClientListener {
}

export type ListenerLookupController = {
    registerListenerForLookup(listenerName: string, listener: ClientListener): void;
    lookupListener(listenerName: string): ClientListener;
};

export type EventsController = {
    readonly on: typeof sp.on, 
    readonly once: typeof sp.once,
    readonly emitter: EventEmitterType
};

export type CombinedController = EventsController & ListenerLookupController;
