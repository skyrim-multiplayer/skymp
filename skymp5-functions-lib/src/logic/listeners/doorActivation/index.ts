import { PlayerController } from '../../PlayerController';
import { GameModeListener } from '../GameModeListener';
import { Mp, PapyrusObject } from '../../../types/mp';
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
    const caster: PapyrusObject = {
      type: "form",
      desc: mp.getDescFromId(casterActorId)
    };

    const base = mp.callPapyrusFunction("method", "ObjectReference", "GetBaseObject", {type: "form", desc: targetObjectDesc}, []);
    const type = mp.callPapyrusFunction("method", "Form", "GetType", base as PapyrusObject, []);
    if (type !== 29) {
      return "continue";
    }
    mp.callPapyrusFunction("global", "SkyMP", "SetDefaultActor", null, [caster]); // todo: should be detected automatically?
    
    (mp.callPapyrusFunction("global", "Game", "GetCameraState", null, []) as unknown as Promise<any>).then((state) => {
      const isFirstPerson = state === 0;
      if (!isFirstPerson) {
        mp.callPapyrusFunction("global", "Debug", "SendAnimationEvent", null, [caster, "IdleActivateDoor"]);
      }
    });

    mp.callPapyrusFunction("global", "SkyMP", "SetDefaultActor", null, [null]);

    return 'continue';
  }
}
