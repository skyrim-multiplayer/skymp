import { EventEmitter } from "eventemitter3";
import { GameLoadEvent } from "./gameLoadEvent";
import { SendMessageEvent } from "./sendMessageEvent";
import { AnyMessage } from "../messages/anyMessage";
import { SendMessageWithRefrIdEvent } from "./sendMessageWithRefrIdEvent";

type EventTypes = {
    'gameLoad': [GameLoadEvent],

    'sendMessage': [SendMessageEvent<AnyMessage>],
    'sendMessageWithRefrId': [SendMessageWithRefrIdEvent<AnyMessage>]
}

// https://blog.makerx.com.au/a-type-safe-event-emitter-in-node-js/
interface TypedEventEmitter<TEvents extends Record<string, any>> {
    emit<TEventName extends keyof TEvents & string>(
        eventName: TEventName,
        ...eventArg: TEvents[TEventName]
    ): void;

    on<TEventName extends keyof TEvents & string>(
        eventName: TEventName,
        handler: (...eventArg: TEvents[TEventName]) => void
    ): void;

    off<TEventName extends keyof TEvents & string>(
        eventName: TEventName,
        handler: (...eventArg: TEvents[TEventName]) => void
    ): void;
}

export type EventEmitterType = TypedEventEmitter<EventTypes>;

export class EventEmitterFactory {
    static makeEventEmitter(): EventEmitterType {
        return (new EventEmitter()) as EventEmitterType;
    }
}
