import * as sp from "skyrimPlatform";

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
    on: typeof sp.on, 
    once: typeof sp.once 
};

export type CombinedController = EventsController & ListenerLookupController;
