import { PlayerController } from './PlayerController';
import { memId, expId, skillRecipes, idBasedData, IPossessedSkills } from './skillMenuData';
import { Mp } from '../types/mp';
import { EvalProperty } from "../props/evalProperty";
import { Ctx } from "../types/ctx";

declare const mp: Mp;
declare const ctx: Ctx;


const discardSkills = (actorId : number, controller: PlayerController, possessedSkills : IPossessedSkills) => {
  let totalExp = 0;
  Object.keys(possessedSkills).forEach(skillName => {
    const skill = possessedSkills[skillName]
    const price = skillRecipes[skillName].slice(0, skill.level + 1).reduce((a, b) => a + b.price, 0);
    totalExp += price;
    controller.removeItem(actorId, skill.id, 1, null);
    controller.addItem(actorId, memId, 1);
  })
  controller.addItem(actorId, expId, Math.round(totalExp/2))
}

export const craftSkill = (actorId: number, controller: PlayerController, argsRaw: string | undefined) => {
  if (!argsRaw) return;
  const [newSkillName, level] = argsRaw.split(' ');
  const inventory = mp.get(actorId, 'inventory').entries;

  //TODO: remove before release
  if (newSkillName === 'mem' && level) {
    controller.addItem(actorId, memId, +level)
    return
  }
  if (newSkillName === 'exp' && level) {
    controller.addItem(actorId, expId, +level)
    return
  }

  if (newSkillName == 'quit') {
    EvalProperty.eval(
      actorId,
      () => {
        ctx.sp.browser.setFocused(false)
      },
    );
    return;
  }

  let memCount = 0;
  let expCount = 0;
  let possessedSkills = {} as IPossessedSkills;
  for (const item of inventory) {
    if (item.baseId === memId) {
      memCount = item.count;
    } else if (item.baseId === expId) {
      expCount = item.count;
    } else if (item.baseId in idBasedData) {
      const skill = idBasedData[item.baseId];
      possessedSkills[skill.name] = {id: item.baseId, level: skill.level, price: skill.price}
    }
  }

  if (newSkillName === 'discard') return discardSkills(actorId, controller, possessedSkills)

  let itemIdToRemove = 0;
  if (level === '0') {
    if (memCount > 0) {
      itemIdToRemove = memId;
    }
  } else {
    const possesedSkill = possessedSkills[newSkillName];
    if (possesedSkill && possesedSkill.level + 1 === +level ) {
      itemIdToRemove = possesedSkill.id;
    }
  }

  if (itemIdToRemove === 0) return;

  const newSkill = skillRecipes[newSkillName][+level]
  const price = newSkill.price;
  if (price > expCount) return;

  controller.removeItem(actorId, itemIdToRemove, 1, null);
  controller.removeItem(actorId, expId, price, null);
  controller.addItem(actorId, newSkill.id, 1);
};
