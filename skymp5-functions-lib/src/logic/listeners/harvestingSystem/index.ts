import { PlayerController } from '../../PlayerController';
import { GameModeListener } from '../GameModeListener';
import { Mp, ServerSettings } from '../../../types/mp';
import { Ctx } from '../../../types/ctx';
import { EvalProperty } from '../../../props/evalProperty';
import { FunctionInfo } from '../../../utils/functionInfo';

declare const mp: Mp;
declare const ctx: Ctx;

export class HarvestingSystem implements GameModeListener {
  constructor(private mp: Mp, private controller: PlayerController) {
    this.serverSettings = this.mp.getServerSettings();
    this.controller

    mp.makeEventSource('_onHarvestingSystem', new FunctionInfo(HarvestingSystem.clientInitEvent()).getText());

    mp['_onHarvestingSystem'] = () => {};
  }

  onPlayerActivateObject(
    casterActorId: number,
    targetObjectDesc: string,
    targetActorId: number
  ): 'continue' | 'blockActivation' {
    console.log(casterActorId, targetObjectDesc, targetActorId);
    // const targetBaseObject = this.mp.get(targetActorId);
    // const testArray = [2];
    // EvalProperty.eval(
    //   casterActorId,
    //   () => {
    //     testArray.push(3);
    //   },
    //   { testArray: testArray }
    // );
    // console.log(testArray);
    return 'continue';
  } 

  private static harvestLogic(actorId: number, controller: PlayerController, argsRaw: string | undefined) {
    console.log(argsRaw);
  }
  private static clientInitEvent() {
    return () => {
    //   const animationName = 'IdleActivatePickUpLow';
    //   ctx.sp.on('activate', (event) => {
    //     const targetBaseObject = event.target.getBaseObject();
    //     if (!targetBaseObject) return;
    //     const targetType = targetBaseObject.getType();
    //     ctx.sp.printConsole(
    //       'gammode harvest',
    //       targetBaseObject.getNthKeyword(0),
    //       targetType,
    //       targetBaseObject.getNumKeywords(),
    //       targetBaseObject.getFormID().toString(16)
    //     );
    //     if (targetType === 39) {
    //       ctx.sp.printConsole('flora activated');
    //     } 
    //     if (targetType === 38) {
    //       const plant = ctx.sp.TreeObject.from(targetBaseObject);
    //       if (!plant) return;
    //       const ingredient = plant.getIngredient();
    //       ctx.sp.Debug.sendAnimationEvent(ctx.sp.Game.getPlayer(), 'IdleActivatePickUpLow');
    //     }
    //   });
    };
  }

  private serverSettings: ServerSettings;
}
