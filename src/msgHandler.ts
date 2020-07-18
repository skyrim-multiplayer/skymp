import * as msg from './messages';

export interface MsgHandler {
    createActor(msg: msg.CreateActorMessage);
    destroyActor(msg: msg.DestroyActorMessage);
    UpdateMovement(msg: msg.UpdateMovementMessage);
    UpdateAnimation(msg: msg.UpdateAnimationMessage);
    UpdateLook(msg: msg.UpdateLookMessage);
    setRaceMenuOpen(msg: msg.SetRaceMenuOpenMessage);

    handleConnectionAccepted();
    handleDisconnect();
}