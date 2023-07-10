import { PlayerController } from '../../PlayerController';
import { EspmLookupResult, Mp, ServerSettings } from '../../../types/mp';
import { Ctx } from '../../../types/ctx';
import { EvalProperty } from '../../../props/evalProperty';
import { FunctionInfo } from '../../../utils/functionInfo';
import { GameModeListener } from '../gameModeListener';
import { getPossessedSkills } from '../commands/skillCommand';

declare const mp: Mp;
declare const ctx: Ctx;

export class HarvestingSystem implements GameModeListener {
  constructor(private mp: Mp, private controller: PlayerController) {
    this.serverSettings = this.mp.getServerSettings();
    mp.makeEventSource('_harvestAnimSystem', new FunctionInfo(HarvestingSystem.clientsideInitEvent()).getText());

    // doesn't really emit anything, just calls the function
    mp['_harvestAnimSystem'] = () => { };
  }

  private serverSettings: ServerSettings;

  private static clientsideInitEvent() {
    return () => {
        if (ctx.sp.storage["harvestAnimSystemInstalled"] !== true) {
            ctx.sp.storage["harvestAnimSystemInstalled"] = true;
        }
        else {
            // Hot reload is not supported for now
            // Just stop the loop here
            ctx.sp.storage.sweetCarryAnimationActive = false;
            return;
        }

        if (typeof ctx.sp.storage.harvestAnimationActive !== "boolean") {
            ctx.sp.storage.harvestAnimationActive = false;
        }

        const playerId: number = 0x14;

        const HARVEST_ANIM_RESTRICT = ['Jump*', 'SprintStart', 'WeapEquip'];
        for (let restrictedAnim of HARVEST_ANIM_RESTRICT) {
          ctx.sp.hooks.sendAnimationEvent.add({
              enter: ((animationEventCtx) => {
                  if (ctx.sp.storage.harvestAnimationActive) {
                      animationEventCtx.animEventName = "";
                  }
              }),
              leave: () => { },
          }, playerId, playerId, restrictedAnim);
        }
    };
}

  private static uint8ToUint32(data: Uint8Array): number[] {
    const uint8Arr = new Uint8Array(data);
    return Array.from(new Uint32Array(uint8Arr.buffer));
  }

  private static getNumberField(lookup: EspmLookupResult | Partial<EspmLookupResult>, fieldName: string): number {
    if (!lookup.record || !lookup.toGlobalRecordId) return NaN;
    const fieldIndex = lookup.record.fields.findIndex((field) => field.type === fieldName);
    if (fieldIndex === -1) return NaN;
    return lookup.toGlobalRecordId(HarvestingSystem.uint8ToUint32(lookup.record.fields[fieldIndex].data)[0]);
  }

  private static checkIngredient(ingredientId: number): {skillType: string[], ids: number[]} | null {
    const isJazbayGrapes = 0x0006ac4a === ingredientId;
    const isIngredientToFood = [0x4b0ba, 0x34d22].includes(ingredientId);

    const lookupResIngredient = mp.lookupEspmRecordById(ingredientId);
    if (!lookupResIngredient.record) return null;

    const keywords:string[] = [];
    const skillType:string[] = [];
    const ids:number[] = [];

    if (lookupResIngredient.record.type === 'LVLI') {
      const LVLOs = lookupResIngredient.record.fields.filter((field) => field.type === 'LVLO')
      LVLOs.forEach((ob) => {
        const obData = HarvestingSystem.uint8ToUint32(ob.data);
        if (!lookupResIngredient.toGlobalRecordId) return [];
        const ingredientLVLOId = lookupResIngredient.toGlobalRecordId(obData[1]);
        const lookupResingredientLVLO = mp.lookupEspmRecordById(ingredientLVLOId);
        if (!lookupResingredientLVLO.record) return [];
        if (lookupResingredientLVLO.record.type === 'INGR') {
          ids.push(ingredientLVLOId);
          const res = HarvestingSystem.checkIngredient(ingredientLVLOId);
          res && res.skillType.forEach(sT => skillType.includes(sT) || skillType.push(sT));
        }
      })
    } else {
      const kwdaIndex = lookupResIngredient.record.fields.findIndex((field) => field.type === 'KWDA');
      if (kwdaIndex === -1) return null;
      const keywordsArray = lookupResIngredient.record.fields[kwdaIndex].data;
      ids.push(ingredientId);
      const keywordIds = HarvestingSystem.uint8ToUint32(keywordsArray);
      keywordIds.forEach(id => {
        if (!lookupResIngredient.toGlobalRecordId) return []
        const keywordId = lookupResIngredient.toGlobalRecordId(id);
        const keywordRecord = mp.lookupEspmRecordById(keywordId).record;
        if (!keywordRecord) return [];
        keywords.push(keywordRecord.editorId);
      })
    }

    if (keywords.includes('VendorItemFood') || isJazbayGrapes || isIngredientToFood) {
      if (!skillType.includes('farmer')) skillType.push('farmer');
    }
    if ((keywords.includes('VendorItemIngredient') && !isIngredientToFood) || isJazbayGrapes) {
      if (!skillType.includes('doctor')) skillType.push('doctor');
    }
    if (isJazbayGrapes) {
      if (!skillType.includes('bee')) skillType.push('bee');
    }

    return {skillType, ids};
  }

  onPlayerActivateObject(
    casterActorId: number,
    targetObjectDesc: string,
    targetId: number
  ): 'continue' | 'blockActivation' {

    const isHarvested = mp.callPapyrusFunction(
      'method',
      'ObjectReference',
      'IsHarvested',
      {desc: targetObjectDesc, type: 'form'},
      []
    );

    if (isHarvested) return 'continue';

    const baseId = HarvestingSystem.getNumberField(mp.lookupEspmRecordById(targetId), 'NAME');
    if (!baseId) return 'continue';

    const ingredientId = HarvestingSystem.getNumberField(mp.lookupEspmRecordById(baseId), 'PFIG');
    if (!ingredientId) return 'continue';

    const res = HarvestingSystem.checkIngredient(ingredientId);
    if (res === null) 'continue';
    const skillType = res ? res.skillType : [];
    const ids = res ? res.ids : [];

    if (skillType.length === 0) return 'continue';
    EvalProperty.eval(casterActorId, () => {
      const animations = ['IdleActivatePickUpLow', 'IdleActivatePickUp'];
      ctx.sp.Debug.sendAnimationEvent(ctx.sp.Game.getPlayer(), animations[Math.random() > 0.5 ? 1 : 0]);
      ctx.sp.storage.harvestAnimationActive = true;
      setTimeout(() => ctx.sp.storage.harvestAnimationActive = false, 2500);
    });

    const { possessedSkills } = getPossessedSkills(casterActorId);
    // 0 level is student, 1 level is adept...
    let maxLevel = -1;
    skillType.forEach((skillName) => {
      if (skillName in possessedSkills) {
        maxLevel = Math.max(possessedSkills[skillName].level, maxLevel);
      }
    });
    if (maxLevel == -1) return 'continue';

    if (ingredientId === 0x00064b3f) return 'continue';

    const isDrawn = mp.callPapyrusFunction(
      'method',
      'Actor',
      'IsWeaponDrawn',
      {desc: mp.getDescFromId(casterActorId), type: 'form'},
      []
    );

    const equipment = mp.get(casterActorId, 'equipment').inv.entries;

    if (ingredientId === 0x4b0ba) {
      const sickle = equipment.find(item => item.baseId === 0x70BAD73);
      if (!(sickle && sickle.worn && isDrawn)) return 'continue';
    }

    if (ingredientId === 0x64B41) {
      const shovel = equipment.find(item => item.baseId === 0x7E870FB);
      if (!(shovel && shovel.worn && isDrawn)) return 'continue';
    }

    const additionalItemsNumber = maxLevel + (Math.random() > 0.5 ? 1 : 0);
    setTimeout(() => {ids.forEach(id =>
      this.controller.addItem(casterActorId, id, additionalItemsNumber, true))
    }, 1000);

    return 'continue';
  }
}
