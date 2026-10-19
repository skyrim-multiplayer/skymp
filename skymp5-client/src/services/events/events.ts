import { EventEmitter } from 'eventemitter3';

import { AnyMessage } from '../messages/anyMessage';
import { ChangeValuesMessage } from '../messages/changeValuesMessage';
import { CreateActorMessage } from '../messages/createActorMessage';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { DeathStateContainerMessage } from '../messages/deathStateContainerMessage';
import { DestroyActorMessage } from '../messages/destroyActorMessage';
import { HostStartMessage } from '../messages/hostStartMessage';
import { HostStopMessage } from '../messages/hostStopMessage';
import { OpenContainerMessage } from '../messages/openContainerMessage';
import { SetInventoryMessage } from '../messages/setInventoryMessage';
import { SetRaceMenuOpenMessage } from '../messages/setRaceMenuOpenMessage';
import { SpSnippetMessage } from '../messages/spSnippetMessage';
import { SpellCastMessage } from '../messages/spellCastMessage';
import { TeleportMessage } from '../messages/teleportMessage';
import { TeleportMessage2 } from '../messages/teleportMessage2';
import { UpdateAnimVariablesMessage } from '../messages/updateAnimVariablesMessage';
import { UpdateAnimationMessage } from '../messages/updateAnimationMessage';
import { UpdateAppearanceMessage } from '../messages/updateAppearanceMessage';
import { UpdateEquipmentMessage } from '../messages/updateEquipmentMessage';
import { UpdateGamemodeDataMessage } from '../messages/updateGameModeDataMessage';
import { UpdateMovementMessage } from '../messages/updateMovementMessage';
import { UpdatePropertyMessage } from '../messages/updatePropertyMessage';
import { AnyRawMessageEvent } from './anyRawMessageEvent';
import { ApplyDeathStateEvent } from './applyDeathStateEvent';
import { AuthAttemptEvent } from './authAttemptEvent';
import { AuthNeededEvent } from './authNeededEvent';
import { BrowserWindowLoadedEvent } from './browserWindowLoadedEvent';
import { ConnectionAccepted } from './connectionAccepted';
import { ConnectionDenied } from './connectionDenied';
import { ConnectionDisconnect } from './connectionDisconnect';
import { ConnectionFailed } from './connectionFailed';
import { ConnectionMessage } from './connectionMessage';
import { GameLoadEvent } from './gameLoadEvent';
import { NewLocalLagValueCalculatedEvent } from './newLocalLagValueCalculatedEvent';
import { NicknameCreateEvent } from './nicknameCreateEvent';
import { NicknameDestroyEvent } from './nicknameDestroyEvent';
import { QueryBlockSetInventoryEvent } from './queryBlockSetInventoryEvent';
import { QueryKeyCodeBindings } from './queryKeyCodeBindings';
import { SendMessageEvent } from './sendMessageEvent';
import { SendMessageWithRefrIdEvent } from './sendMessageWithRefrIdEvent';
import { SendRawMessageEvent } from './sendRawMessageEvent';

type EventTypes = {
  gameLoad: [GameLoadEvent];

  sendMessage: [SendMessageEvent<AnyMessage>];
  sendRawMessage: [SendRawMessageEvent];
  sendMessageWithRefrId: [SendMessageWithRefrIdEvent<AnyMessage>];

  applyDeathStateEvent: [ApplyDeathStateEvent];

  connectionFailed: [ConnectionFailed];
  connectionDenied: [ConnectionDenied];
  connectionAccepted: [ConnectionAccepted];
  connectionDisconnect: [ConnectionDisconnect];

  updateMovementMessage: [ConnectionMessage<UpdateMovementMessage>];
  updateAnimationMessage: [ConnectionMessage<UpdateAnimationMessage>];
  updateEquipmentMessage: [ConnectionMessage<UpdateEquipmentMessage>];
  changeValuesMessage: [ConnectionMessage<ChangeValuesMessage>];
  spellCastMessage: [ConnectionMessage<SpellCastMessage>];
  updateAnimVariablesMessage: [ConnectionMessage<UpdateAnimVariablesMessage>];
  updateAppearanceMessage: [ConnectionMessage<UpdateAppearanceMessage>];
  teleportMessage: [ConnectionMessage<TeleportMessage>];
  openContainerMessage: [ConnectionMessage<OpenContainerMessage>];
  hostStartMessage: [ConnectionMessage<HostStartMessage>];
  hostStopMessage: [ConnectionMessage<HostStopMessage>];
  setInventoryMessage: [ConnectionMessage<SetInventoryMessage>];
  createActorMessage: [ConnectionMessage<CreateActorMessage>];
  destroyActorMessage: [ConnectionMessage<DestroyActorMessage>];
  setRaceMenuOpenMessage: [ConnectionMessage<SetRaceMenuOpenMessage>];
  spSnippetMessage: [ConnectionMessage<SpSnippetMessage>];
  updateGamemodeDataMessage: [ConnectionMessage<UpdateGamemodeDataMessage>];
  updatePropertyMessage: [ConnectionMessage<UpdatePropertyMessage>];
  deathStateContainerMessage: [ConnectionMessage<DeathStateContainerMessage>];
  teleportMessage2: [ConnectionMessage<TeleportMessage2>];
  customPacketMessage: [ConnectionMessage<CustomPacketMessage>];

  browserWindowLoaded: [BrowserWindowLoadedEvent];
  authAttempt: [AuthAttemptEvent];
  authNeeded: [AuthNeededEvent];
  anyMessage: [ConnectionMessage<AnyMessage>];
  anyRawMessage: [AnyRawMessageEvent];
  newLocalLagValueCalculated: [NewLocalLagValueCalculatedEvent];
  queryBlockSetInventoryEvent: [QueryBlockSetInventoryEvent];
  queryKeyCodeBindings: [QueryKeyCodeBindings];
  nicknameCreate: [NicknameCreateEvent];
  nicknameDestroy: [NicknameDestroyEvent];
};

// https://blog.makerx.com.au/a-type-safe-event-emitter-in-node-js/
interface TypedEventEmitter<TEvents extends Record<string, any>> {
  emit<TEventName extends keyof TEvents & string>(
    eventName: TEventName,
    ...eventArg: TEvents[TEventName]
  ): void;

  on<TEventName extends keyof TEvents & string>(
    eventName: TEventName,
    handler: (...eventArg: TEvents[TEventName]) => void,
  ): void;

  off<TEventName extends keyof TEvents & string>(
    eventName: TEventName,
    handler: (...eventArg: TEvents[TEventName]) => void,
  ): void;
}

export type EventEmitterType = TypedEventEmitter<EventTypes>;

export class EventEmitterFactory {
  static makeEventEmitter(): EventEmitterType {
    return new EventEmitter() as EventEmitterType;
  }
}
