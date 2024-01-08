import { EventEmitter } from "eventemitter3";
import { GameLoadEvent } from "./gameLoadEvent";
import { SendMessageEvent } from "./sendMessageEvent";
import { AnyMessage } from "../messages/anyMessage";
import { SendMessageWithRefrIdEvent } from "./sendMessageWithRefrIdEvent";
import { ApplyDeathStateEvent } from "./applyDeathStateEvent";
import { ConnectionFailed } from "./connectionFailed";
import { ConnectionDenied } from "./connectionDenied";
import { ConnectionAccepted } from "./connectionAccepted";
import { ConnectionDisconnect } from "./connectionDisconnect";
import { ConnectionMessage } from "./connectionMessage";
import { HostStartMessage } from "../messages/hostStartMessage";
import { HostStopMessage } from "../messages/hostStopMessage";
import { SetInventoryMessage } from "../messages/setInventoryMessage";
import { OpenContainerMessage } from "../messages/openContainerMessage";
import { ChangeValuesMessage } from "../messages/changeValues";
import { CreateActorMessage } from "../messages/createActorMessage";
import { CustomPacketMessage2 } from "../messages/customPacketMessage2";
import { DestroyActorMessage } from "../messages/destroyActorMessage";
import { SetRaceMenuOpenMessage } from "../messages/setRaceMenuOpenMessage";
import { SpSnippetMessage } from "../messages/spSnippetMessage";
import { TeleportMessage } from "../messages/teleportMessage";
import { UpdateAnimationMessage } from "../messages/updateAnimationMessage";
import { UpdateAppearanceMessage } from "../messages/updateAppearanceMessage";
import { UpdateEquipmentMessage } from "../messages/updateEquipmentMessage";
import { UpdateGamemodeDataMessage } from "../messages/updateGameModeDataMessage";
import { UpdateMovementMessage } from "../messages/updateMovementMessage";
import { UpdatePropertyMessage } from "../messages/updatePropertyMessage";
import { DeathStateContainerMessage } from "../messages/deathStateContainerMessage";
import { TeleportMessage2 } from "../messages/teleportMessage2";
import { BrowserWindowLoadedEvent } from "./browserWindowLoadedEvent";
import { AuthEvent } from "./authEvent";
import { NewLocalLagValueCalculatedEvent } from "./newLocalLagValueCalculatedEvent";
import { AuthNeededEvent } from "./authNeededEvent";

type EventTypes = {
    'gameLoad': [GameLoadEvent],

    'sendMessage': [SendMessageEvent<AnyMessage>],
    'sendMessageWithRefrId': [SendMessageWithRefrIdEvent<AnyMessage>],

    'applyDeathStateEvent': [ApplyDeathStateEvent],

    'connectionFailed': [ConnectionFailed],
    'connectionDenied': [ConnectionDenied],
    'connectionAccepted': [ConnectionAccepted],
    'connectionDisconnect': [ConnectionDisconnect],

    'updateMovementMessage': [ConnectionMessage<UpdateMovementMessage>],
    'updateAnimationMessage': [ConnectionMessage<UpdateAnimationMessage>],
    'updateEquipmentMessage': [ConnectionMessage<UpdateEquipmentMessage>],
    'changeValuesMessage': [ConnectionMessage<ChangeValuesMessage>],
    'updateAppearanceMessage': [ConnectionMessage<UpdateAppearanceMessage>],
    'teleportMessage': [ConnectionMessage<TeleportMessage>],
    'openContainerMessage': [ConnectionMessage<OpenContainerMessage>],
    'hostStartMessage': [ConnectionMessage<HostStartMessage>],
    'hostStopMessage': [ConnectionMessage<HostStopMessage>],
    'setInventoryMessage': [ConnectionMessage<SetInventoryMessage>],
    'createActorMessage': [ConnectionMessage<CreateActorMessage>],
    'customPacketMessage2': [ConnectionMessage<CustomPacketMessage2>],
    'destroyActorMessage': [ConnectionMessage<DestroyActorMessage>],
    'setRaceMenuOpenMessage': [ConnectionMessage<SetRaceMenuOpenMessage>],
    'spSnippetMessage': [ConnectionMessage<SpSnippetMessage>],
    'updateGamemodeDataMessage': [ConnectionMessage<UpdateGamemodeDataMessage>],
    'updatePropertyMessage': [ConnectionMessage<UpdatePropertyMessage>],
    'deathStateContainerMessage': [ConnectionMessage<DeathStateContainerMessage>],
    'teleportMessage2': [ConnectionMessage<TeleportMessage2>]

    'browserWindowLoaded': [BrowserWindowLoadedEvent],
    'auth': [AuthEvent],
    'authNeeded': [AuthNeededEvent],
    'anyMessage': [ConnectionMessage<AnyMessage>],
    'newLocalLagValueCalculated': [NewLocalLagValueCalculatedEvent]
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
