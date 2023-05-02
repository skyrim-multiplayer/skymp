import { PlayerController } from '../../PlayerController';
import { GameModeListener } from '../GameModeListener';
import { Mp, ServerSettings } from '../../../types/mp';
import { Ctx } from '../../../types/ctx';
import { EvalProperty } from '../../../props/evalProperty';

declare const mp: Mp;
declare const ctx: Ctx;

export class HarvestingSystem implements GameModeListener {
  constructor(private mp: Mp, private controller: PlayerController) {
    this.serverSettings = this.mp.getServerSettings();
  }

  private serverSettings: ServerSettings;

  private static uint8ToNumber(data: Uint8Array): number {
    const uint8Arr = new Uint8Array(data);
    // TODO: call toGlobalRecordId on received id, won't work with mod items
    return new Uint32Array(uint8Arr.buffer)[0];
  }

  onPlayerActivateObject(
    casterActorId: number,
    targetObjectDesc: string,
    targetId: number
  ): 'continue' | 'blockActivation' {
    const lookupRes = mp.lookupEspmRecordById(targetId);
    if (!lookupRes.record || !lookupRes.toGlobalRecordId) return 'continue';
    const nameIndex = lookupRes.record.fields.findIndex((field) => field.type === 'NAME');
    if (nameIndex === -1) return 'continue';
    const baseId = lookupRes.toGlobalRecordId(HarvestingSystem.uint8ToNumber(lookupRes.record.fields[nameIndex].data));

    const lookupResBase = mp.lookupEspmRecordById(baseId);
    if (!lookupResBase.record || !lookupResBase.toGlobalRecordId) return 'continue';
    const pfigIndex = lookupResBase.record.fields.findIndex((field) => field.type === 'PFIG');
    if (pfigIndex === -1) return 'continue';
    const ingredientId = lookupResBase.toGlobalRecordId(HarvestingSystem.uint8ToNumber(lookupResBase.record.fields[pfigIndex].data));
    const isJazbayGrapes = 0x0006ac4a === ingredientId;
    const isIngredientToFood = [0x4b0ba, 0x34d22].includes(ingredientId);

    const skillType = [];

    const lookupResIngredient = mp.lookupEspmRecordById(ingredientId);
    if (!lookupResIngredient.record || !lookupResIngredient.toGlobalRecordId) return 'continue';
    const kwdaIndex = lookupResIngredient.record.fields.findIndex((field) => field.type === 'KWDA');
    if (kwdaIndex === -1) return 'continue';
    const keywordsArray = lookupResIngredient.record.fields[kwdaIndex].data;
    const importantKeywords = [];
    for (let i = 0; i < keywordsArray.length / 4; i++) {
      const keywordId = lookupResIngredient.toGlobalRecordId(HarvestingSystem.uint8ToNumber(
        lookupResIngredient.record.fields[kwdaIndex].data.slice(i * 4, (i + 1) * 4)
      ));
      const keywordRecord = mp.lookupEspmRecordById(keywordId).record;
      if (!keywordRecord) return 'continue';
      if (keywordRecord.editorId === 'VendorItemFood' || keywordRecord.editorId === 'VendorItemIngredient') {
        importantKeywords.push(keywordRecord.editorId);
      }
    }
    if (importantKeywords.includes('VendorItemFood') || isJazbayGrapes || isIngredientToFood) {
      skillType.push('farmer');
    }
    if (importantKeywords.includes('VendorItemIngredient') && !isIngredientToFood) {
      skillType.push('doctor');
    }
    if (isJazbayGrapes) {
      skillType.push('bee');
    }

    console.log(skillType);
    if (skillType.length === 0) return 'continue';

    EvalProperty.eval(casterActorId, () => {
        const animations = ['IdleActivatePickUpLow', 'IdleActivatePickUp'];
        ctx.sp.Debug.sendAnimationEvent(ctx.sp.Game.getPlayer(), animations[(Math.random() > 0.5) ? 1 : 0]);
    });

    return 'blockActivation';
  }
}
