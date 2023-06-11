import { PlayerController } from '../../PlayerController';
import { GameModeListener } from '../GameModeListener';
import { Mp } from '../../../types/mp';
import { Ctx } from '../../../types/ctx';
import { EvalProperty } from '../../../props/evalProperty';

declare const mp: Mp;
declare const ctx: Ctx;
declare const target: number;

export class DoorActivation implements GameModeListener {
  constructor(private mp: Mp, private controller: PlayerController) {}

  onPlayerActivateObject(
    casterActorId: number,
    targetObjectDesc: string,
    targetId: number
  ): 'continue' | 'blockActivation' {
    EvalProperty.eval(
      casterActorId,
      () => {
        const form = ctx.sp.Game.getForm(target);
        if (form) {
          const targetReference = ctx.sp.ObjectReference.from(form);
          if (targetReference) {;
            const baseObject = targetReference.getBaseObject();
            if (baseObject) {
              if (baseObject.getType() === 29)
                ctx.sp.Debug.sendAnimationEvent(ctx.sp.Game.getPlayer(), 'IdleActivateDoor');
            }
          }
        }
      },
      { target: targetId }
    );

    return 'continue';
  }
}
