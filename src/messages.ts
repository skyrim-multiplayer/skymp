import { Movement, Transform } from './components/movement';
import { Animation } from './components/animation';

export enum MsgType {
    CustomPacket = 1,
    UpdateMovement = 2,
    UpdateAnimation = 3
};

export interface CreateActorMessage {
    type: 'createActor';
    idx: number;
    transform: Transform;
    isMe: boolean;
}

export interface DestroyActorMessage {
    type: 'destroyActor';
    idx: number;
}

export interface UpdateMovementMessage {
    t: MsgType.UpdateMovement;
    idx: number;
    data: Movement;
}

export interface UpdateAnimationMessage {
    t: MsgType.UpdateAnimation;
    idx: number;
    data: Animation;
}