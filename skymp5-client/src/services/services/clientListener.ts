import * as sp from "skyrimPlatform";
import { EventEmitterType } from "../events/events";

export interface ClientListenerEvents { 
    on: typeof sp.on, 
    once: typeof sp.once 
};

export type Sp = Omit<typeof sp, "on" | "once">;

export abstract class ClientListener {
    // TODO: redirect this to spdlog
    protected logError(error: string) {
        sp.printConsole(`Error in ${this.constructor.name}:`, error);
    }

    // TODO: redirect this to spdlog
    protected logTrace(trace: string) {
        sp.printConsole(`Trace in ${this.constructor.name}:`, trace);
    }
}

export type ClientListenerConstructor<T> = {
    new(...args: any[]): T;
    readonly name: string;
}

export type ListenerLookupController = {
    lookupListener<T extends ClientListener>(constructor: ClientListenerConstructor<T>): T;
};

export type EventsController = {
    readonly on: typeof sp.on, 
    readonly once: typeof sp.once,
    readonly emitter: EventEmitterType
};

export type CombinedController = EventsController & ListenerLookupController;
