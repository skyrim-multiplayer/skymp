import { Mp } from "../../../types/mp";
import { PlayerController } from "../../PlayerController";
import { Command, HandlerInput } from "./command";
import { EvalProperty } from "../../../props/evalProperty";
import { Ctx } from "../../../types/ctx";

declare const mp: Mp;
declare const ctx: Ctx;
declare const frontData: string;

export class SkillCommand extends Command {
    constructor(mp: Mp, controller: PlayerController) {
        super(mp, controller, "skill");
    }

    handle(input: HandlerInput): void {
        const { actorId, controller, argsRaw } = input;
        craftSkill(actorId, controller, argsRaw);
    }
}

const discardSkills = (actorId: number, controller: PlayerController, possessedSkills: IPossessedSkills, expCount: number) => {
    if (expCount > 500) {
        controller.removeItem(actorId, expId, expCount - 500, null, true);
    }
    let totalExp = 0;
    Object.keys(possessedSkills).forEach(skillName => {
        const skill = possessedSkills[skillName]
        const price = skillRecipes[skillName].slice(0, skill.level + 1).reduce((a, b) => a + b.price, 0);
        totalExp += price;
        controller.removeItem(actorId, skill.id, 1, null, true);
    })
    controller.addItem(actorId, expId, Math.round(totalExp / 2), true)
}

export const getPossessedSkills = (actorId: number, controller?: PlayerController) => {
    const possessedSkills = {} as IPossessedSkills;
    let memCount = 1000;
    let expCount = 0;
    const inventory = mp.get(actorId, 'inventory').entries;
    for (const item of inventory) {
        if (item.baseId === expId) {
            expCount = Math.min(1000, item.count);
            if (expCount > 1000 && controller) {
                controller.removeItem(actorId, expId, expCount - 1000, null, true);
            }
        } else if (item.baseId in idBasedData) {
            const skill = idBasedData[item.baseId];
            possessedSkills[skill.name] = { id: item.baseId, level: skill.level, price: skill.price }
            memCount -= skillRecipes[skill.name].slice(0, skill.level + 1).reduce((a, b) => a + b.price, 0)
        }
    }
    return { possessedSkills, memCount, expCount }
}

export const craftSkill = (actorId: number, controller: PlayerController, argsRaw: string | undefined) => {
    if (!argsRaw) return;
    const [newSkillName, level] = argsRaw.split(' ');

    if (newSkillName === 'init') {
        const { possessedSkills, memCount, expCount } = getPossessedSkills(actorId, controller);
        const perks = {} as { [key: string]: number };
        Object.keys(possessedSkills).forEach(key => perks[key] = possessedSkills[key].level + 1)
        const payload = {
            exp: expCount,
            mem: memCount,
            perks
        }
        EvalProperty.eval(
            actorId,
            () => {
                const src = `
        window.dispatchEvent(new CustomEvent('updateSkillMenu', { detail: ${JSON.stringify(frontData)}}))
        `
                ctx.sp.browser.executeJavaScript(src)
            },
            { frontData: JSON.stringify(payload) }
        );
        return;
    }

    if (newSkillName === 'quit') {
        EvalProperty.eval(
            actorId,
            () => {
                ctx.sp.browser.setFocused(false);
            },
        );
        return;
    }

    const { possessedSkills, memCount, expCount } = getPossessedSkills(actorId);
    if (newSkillName === 'discard') return discardSkills(actorId, controller, possessedSkills, expCount)

    let prevSkillIdToRemove = 0;
    const possessedSkill = possessedSkills[newSkillName];
    if (possessedSkill && possessedSkill.level + 1 === +level) {
        prevSkillIdToRemove = possessedSkill.id;
    }


    const newSkill = skillRecipes[newSkillName][+level]
    const price = newSkill.price;
    if (price > expCount || price > memCount) return;

    if (prevSkillIdToRemove) {
        controller.removeItem(actorId, prevSkillIdToRemove, 1, null);
    }
    controller.removeItem(actorId, expId, price, null, true);
    controller.addItem(actorId, newSkill.id, 1, true);
};

interface ISkillRecipes {
    [name: string]: { id: number; price: number }[];
}

export const skillRecipes = {
    woodcutter: [
        { id: 0x7f4ced9, price: 50 },
        { id: 0x7f4ceda, price: 80 },
        { id: 0x7f4cedb, price: 110 },
        { id: 0x7f4cedc, price: 140 },
    ],
    fishman: [
        { id: 0x7f4cedd, price: 50 },
        { id: 0x7f4cede, price: 70 },
        { id: 0x7f4cedf, price: 90 },
        { id: 0x7f4cee0, price: 110 },
    ],
    miner: [
        { id: 0x7e595b2, price: 50 },
        { id: 0x7e595b3, price: 80 },
        { id: 0x7e595b4, price: 110 },
        { id: 0x7e595b5, price: 140 },
    ],
    bee: [
        { id: 0x7f4cee5, price: 50 },
        { id: 0x7f4cee6, price: 80 },
        { id: 0x7f4cee7, price: 110 },
        { id: 0x7f4cee8, price: 140 },
    ],
    saltmaker: [
        { id: 0x7f4ced5, price: 50 },
        { id: 0x7f4ced6, price: 80 },
        { id: 0x7f4ced7, price: 110 },
        { id: 0x7f4ced8, price: 140 },
    ],
    hunter: [
        { id: 0x7f7074d, price: 50 },
        { id: 0x7f7074e, price: 70 },
        { id: 0x7f7074f, price: 90 },
        { id: 0x7f70750, price: 110 },
    ],
    doctor: [
        { id: 0x7f4cee1, price: 50 },
        { id: 0x7f4cee2, price: 70 },
        { id: 0x7f4cee3, price: 90 },
        { id: 0x7f4cee4, price: 110 },
    ],
    farmer: [
        { id: 0x7f4cee9, price: 50 },
        { id: 0x7f4ceea, price: 80 },
        { id: 0x7f4ceeb, price: 110 },
        { id: 0x7f4ceec, price: 140 },
    ],
    armor: [
        { id: 0x7f70751, price: 50 },
        { id: 0x7f70752, price: 80 },
        { id: 0x7f70753, price: 110 },
        { id: 0x7f70754, price: 280 },
    ],
    weapon: [
        { id: 0x7f70755, price: 50 },
        { id: 0x7f70756, price: 80 },
        { id: 0x7f70757, price: 110 },
        { id: 0x7f70758, price: 280 },
    ],
    jewelry: [
        { id: 0x7f70759, price: 50 },
        { id: 0x7f7075a, price: 80 },
        { id: 0x7f7075b, price: 110 },
        { id: 0x7f7075c, price: 140 },
    ],
    leather: [
        { id: 0x7f7075d, price: 50 },
        { id: 0x7f7075e, price: 80 },
        { id: 0x7f7075f, price: 110 },
        { id: 0x7f70760, price: 140 },
    ],
    clother: [
        { id: 0x7f70761, price: 50 },
        { id: 0x7f70762, price: 80 },
        { id: 0x7f70763, price: 110 },
        { id: 0x7f70764, price: 140 },
    ],
    carpenter: [
        { id: 0x7f70769, price: 50 },
        { id: 0x7f7076a, price: 80 },
        { id: 0x7f7076b, price: 110 },
        { id: 0x7f7076c, price: 140 },
    ],
    somelie: [
        { id: 0x7f7076d, price: 50 },
        { id: 0x7f7076e, price: 80 },
        { id: 0x7f7076f, price: 110 },
        { id: 0x7f70770, price: 140 },
    ],
    cheif: [
        { id: 0x7f70765, price: 50 },
        { id: 0x7f70766, price: 80 },
        { id: 0x7f70767, price: 110 },
        { id: 0x7f70768, price: 140 },
    ],
    daggers: [
        { id: 0x7f75871, price: 50 },
        { id: 0x7f75872, price: 80 },
        { id: 0x7f75873, price: 110 },
        { id: 0x7f75874, price: 140 },
    ],
    shortswords: [
        { id: 0x7f75881, price: 50 },
        { id: 0x7f75882, price: 80 },
        { id: 0x7f75883, price: 110 },
        { id: 0x7f75884, price: 140 },
    ],
    swords: [
        { id: 0x7f75875, price: 50 },
        { id: 0x7f75876, price: 80 },
        { id: 0x7f75877, price: 110 },
        { id: 0x7f75878, price: 140 },
    ],
    scimitar: [
        { id: 0x7f75885, price: 50 },
        { id: 0x7f75886, price: 80 },
        { id: 0x7f75887, price: 110 },
        { id: 0x7f75888, price: 280 },
    ],
    katana: [
        { id: 0x7f75879, price: 50 },
        { id: 0x7f7587a, price: 80 },
        { id: 0x7f7587b, price: 110 },
        { id: 0x7f7587c, price: 280 },
    ],
    mace: [
        { id: 0x7f75889, price: 50 },
        { id: 0x7f7588a, price: 80 },
        { id: 0x7f7588b, price: 110 },
        { id: 0x7f7588c, price: 140 },
    ],
    axes: [
        { id: 0x7f7587d, price: 50 },
        { id: 0x7f7587e, price: 80 },
        { id: 0x7f7587f, price: 110 },
        { id: 0x7f75880, price: 140 },
    ],
    hammer: [
        { id: 0x7f7588d, price: 50 },
        { id: 0x7f7588e, price: 80 },
        { id: 0x7f7588f, price: 110 },
        { id: 0x7f75890, price: 140 },
    ],
    bows: [
        { id: 0x7f7a991, price: 50 },
        { id: 0x7f7a992, price: 80 },
        { id: 0x7f7a993, price: 110 },
        { id: 0x7f7a994, price: 140 },
    ],
    longsword: [
        { id: 0x7f7a9a1, price: 50 },
        { id: 0x7f7a9a2, price: 80 },
        { id: 0x7f7a9a3, price: 110 },
        { id: 0x7f7a9a4, price: 140 },
    ],
    greatkatana: [
        { id: 0x7f7a995, price: 50 },
        { id: 0x7f7a996, price: 80 },
        { id: 0x7f7a997, price: 110 },
        { id: 0x7f7a998, price: 280 },
    ],
    battleaxe: [
        { id: 0x7f7a9a5, price: 50 },
        { id: 0x7f7a9a6, price: 80 },
        { id: 0x7f7a9a7, price: 110 },
        { id: 0x7f7a9a8, price: 140 },
    ],
    warhammer: [
        { id: 0x7f7a999, price: 50 },
        { id: 0x7f7a99a, price: 80 },
        { id: 0x7f7a99b, price: 110 },
        { id: 0x7f7a99c, price: 140 },
    ],
    staff: [
        { id: 0x7f7a9a9, price: 50 },
        { id: 0x7f7a9aa, price: 80 },
        { id: 0x7f7a9ab, price: 110 },
        { id: 0x7f7a9ac, price: 140 },
    ],
    pike: [
        { id: 0x7f7a99d, price: 50 },
        { id: 0x7f7a99e, price: 80 },
        { id: 0x7f7a99f, price: 110 },
        { id: 0x7f7a9a0, price: 140 },
    ],
    halberd: [
        { id: 0x7f7a9ad, price: 50 },
        { id: 0x7f7a9ae, price: 80 },
        { id: 0x7f7a9af, price: 110 },
        { id: 0x7f7a9b0, price: 140 },
    ],
    armorlight: [
        { id: 0x7f7fab1, price: 50 },
        { id: 0x7f7fab2, price: 80 },
    ],
    armorheavy: [
        { id: 0x7f7fab3, price: 80 },
        { id: 0x7f7fab4, price: 110 },
    ],
    shield: [
        { id: 0x7f7fab5, price: 50 },
        { id: 0x7f7fab6, price: 80 },
        { id: 0x7f7fab7, price: 110 },
        { id: 0x7f7fab8, price: 140 },
    ],
    conjuration: [
        { id: 0x7f7fac5, price: 50 },
        { id: 0x7f7fac6, price: 80 },
        { id: 0x7f7fac7, price: 110 },
        { id: 0x7f7fac8, price: 630 },
    ],
    alchemy: [
        { id: 0x7f7fab9, price: 50 },
        { id: 0x7f7faba, price: 80 },
        { id: 0x7f7fabb, price: 110 },
        { id: 0x7f7fabc, price: 140 },
    ],
    enchant: [
        { id: 0x7f7fac9, price: 50 },
        { id: 0x7f7faca, price: 80 },
        { id: 0x7f7facb, price: 110 },
        { id: 0x7f7facc, price: 410 },
    ],
    alteration: [
        { id: 0x7f7fabd, price: 50 },
        { id: 0x7f7fabe, price: 80 },
        { id: 0x7f7fabf, price: 110 },
        { id: 0x7f7fac0, price: 630 },
    ],
    destruction: [
        { id: 0x7f7facd, price: 50 },
        { id: 0x7f7face, price: 80 },
        { id: 0x7f7facf, price: 110 },
        { id: 0x7f7fad0, price: 630 },
    ],
    illusion: [
        { id: 0x7f7fac1, price: 50 },
        { id: 0x7f7fac2, price: 80 },
        { id: 0x7f7fac3, price: 110 },
        { id: 0x7f7fac4, price: 410 },
    ],
    restoration: [
        { id: 0x7f7fad1, price: 50 },
        { id: 0x7f7fad2, price: 80 },
        { id: 0x7f7fad3, price: 110 },
        { id: 0x7f7fad4, price: 410 },
    ],
} as ISkillRecipes;
export const expId = 0x7f33922;
// export const memId = 0x700de02;

interface IIdSkill {
    [key: number]: {
        name: string;
        level: number;
        price: number;
    }
}

const convertData = (data: ISkillRecipes) => {
    const res = {} as IIdSkill;
    for (const skill of Object.keys(data)) {
        data[skill].forEach((item, index) => {
            res[item.id] = { name: skill, level: index, price: item.price }
        });
    }
    return res;
}

export const idBasedData = convertData(skillRecipes);

export interface IPossessedSkills {
    [key: string]: {
        id: number;
        level: number;
        price: number;
    }
}
