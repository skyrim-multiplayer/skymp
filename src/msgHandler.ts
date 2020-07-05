import { CreateActorMessage, DestroyActorMessage, UpdateMovementMessage, UpdateAnimationMessage } from './messages';

export interface MsgHandler {
    createActor(msg: CreateActorMessage);
    destroyActor(msg: DestroyActorMessage);
    UpdateMovement(msg: UpdateMovementMessage);
    UpdateAnimation(msg: UpdateAnimationMessage);

    handleConnectionAccepted();
    handleDisconnect();
}