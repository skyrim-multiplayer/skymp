import { browser, Game, on } from 'skyrimPlatform'

import { getInventory } from '../sync/inventory';

interface IItemsIds {
    [id: number]: { name: string, level: number }
}

interface IFrontData {
    exp: number, mem: number, perks: {
        [skill: string]: number,
    }
}

export const skillMenuInit = () => {
    const itemsIds = {
        0x7f4ced9: { name: 'woodcutter', level: 1 },
        0x7f4ceda: { name: 'woodcutter', level: 2 },
        0x7f4cedb: { name: 'woodcutter', level: 3 },
        0x7f4cedc: { name: 'woodcutter', level: 4 },

        0x7f4cedd: { name: 'fishman', level: 1 },
        0x7f4cede: { name: 'fishman', level: 2 },
        0x7f4cedf: { name: 'fishman', level: 3 },
        0x7f4cee0: { name: 'fishman', level: 4 },

        0x7e595b2: { name: 'miner', level: 1 },
        0x7e595b3: { name: 'miner', level: 2 },
        0x7e595b4: { name: 'miner', level: 3 },
        0x7e595b5: { name: 'miner', level: 4 },

        0x7f4cee5: { name: 'bee', level: 1 },
        0x7f4cee6: { name: 'bee', level: 2 },
        0x7f4cee7: { name: 'bee', level: 3 },
        0x7f4cee8: { name: 'bee', level: 4 },

        0x7f4ced5: { name: 'saltmaker', level: 1 },
        0x7f4ced6: { name: 'saltmaker', level: 2 },
        0x7f4ced7: { name: 'saltmaker', level: 3 },
        0x7f4ced8: { name: 'saltmaker', level: 4 },

        0x7f7074d: { name: 'hunter', level: 1 },
        0x7f7074e: { name: 'hunter', level: 2 },
        0x7f7074f: { name: 'hunter', level: 3 },
        0x7f70750: { name: 'hunter', level: 4 },

        0x7f4cee1: { name: 'doctor', level: 1 },
        0x7f4cee2: { name: 'doctor', level: 1 },
        0x7f4cee3: { name: 'doctor', level: 1 },
        0x7f4cee4: { name: 'doctor', level: 1 },

        0x7f4cee9: { name: 'farmer', level: 1 },
        0x7f4ceea: { name: 'farmer', level: 2 },
        0x7f4ceeb: { name: 'farmer', level: 3 },
        0x7f4ceec: { name: 'farmer', level: 4 },

        0x7f70751: { name: 'armor', level: 1 },
        0x7f70752: { name: 'armor', level: 2 },
        0x7f70753: { name: 'armor', level: 3 },
        0x7f70754: { name: 'armor', level: 4 },

        0x7f70755: { name: 'weapon', level: 1 },
        0x7f70756: { name: 'weapon', level: 2 },
        0x7f70757: { name: 'weapon', level: 3 },
        0x7f70758: { name: 'weapon', level: 4 },

        0x7f70759: { name: 'jewelry', level: 1 },
        0x7f7075a: { name: 'jewelry', level: 2 },
        0x7f7075b: { name: 'jewelry', level: 3 },
        0x7f7075c: { name: 'jewelry', level: 4 },

        0x7f7075d: { name: 'leather', level: 1 },
        0x7f7075e: { name: 'leather', level: 2 },
        0x7f7075f: { name: 'leather', level: 3 },
        0x7f70760: { name: 'leather', level: 4 },

        0x7f70761: { name: 'clother', level: 1 },
        0x7f70762: { name: 'clother', level: 2 },
        0x7f70763: { name: 'clother', level: 3 },
        0x7f70764: { name: 'clother', level: 4 },

        0x7f70769: { name: 'carpenter', level: 1 },
        0x7f7076a: { name: 'carpenter', level: 2 },
        0x7f7076b: { name: 'carpenter', level: 3 },
        0x7f7076c: { name: 'carpenter', level: 4 },

        0x7f7076d: { name: 'somelie', level: 1 },
        0x7f7076e: { name: 'somelie', level: 2 },
        0x7f7076f: { name: 'somelie', level: 3 },
        0x7f70770: { name: 'somelie', level: 4 },

        0x7f70765: { name: 'cheif', level: 1 },
        0x7f70766: { name: 'cheif', level: 2 },
        0x7f70767: { name: 'cheif', level: 3 },
        0x7f70768: { name: 'cheif', level: 4 },

        0x7f75871: { name: 'daggers', level: 1 },
        0x7f75872: { name: 'daggers', level: 2 },
        0x7f75873: { name: 'daggers', level: 3 },
        0x7f75874: { name: 'daggers', level: 4 },

        0x7f75881: { name: 'shortswords', level: 1 },
        0x7f75882: { name: 'shortswords', level: 2 },
        0x7f75883: { name: 'shortswords', level: 3 },
        0x7f75884: { name: 'shortswords', level: 4 },

        0x7f75875: { name: 'swords', level: 1 },
        0x7f75876: { name: 'swords', level: 2 },
        0x7f75877: { name: 'swords', level: 3 },
        0x7f75878: { name: 'swords', level: 4 },

        0x7f75885: { name: 'scimitar', level: 1 },
        0x7f75886: { name: 'scimitar', level: 2 },
        0x7f75887: { name: 'scimitar', level: 3 },
        0x7f75888: { name: 'scimitar', level: 4 },

        0x7f75879: { name: 'katana', level: 1 },
        0x7f7587a: { name: 'katana', level: 2 },
        0x7f7587b: { name: 'katana', level: 3 },
        0x7f7587c: { name: 'katana', level: 4 },

        0x7f75889: { name: 'mace', level: 1 },
        0x7f7588a: { name: 'mace', level: 2 },
        0x7f7588b: { name: 'mace', level: 3 },
        0x7f7588c: { name: 'mace', level: 4 },

        0x7f7587d: { name: 'axes', level: 1 },
        0x7f7587e: { name: 'axes', level: 2 },
        0x7f7587f: { name: 'axes', level: 3 },
        0x7f75880: { name: 'axes', level: 4 },

        0x7f7588d: { name: 'hammer', level: 1 },
        0x7f7588e: { name: 'hammer', level: 2 },
        0x7f7588f: { name: 'hammer', level: 3 },
        0x7f75890: { name: 'hammer', level: 4 },

        0x7f7a991: { name: 'bows', level: 1 },
        0x7f7a992: { name: 'bows', level: 2 },
        0x7f7a993: { name: 'bows', level: 3 },
        0x7f7a994: { name: 'bows', level: 4 },

        0x7f7a9a1: { name: 'longsword', level: 1 },
        0x7f7a9a2: { name: 'longsword', level: 2 },
        0x7f7a9a3: { name: 'longsword', level: 3 },
        0x7f7a9a4: { name: 'longsword', level: 4 },

        0x7f7a995: { name: 'greatkatana', level: 1 },
        0x7f7a996: { name: 'greatkatana', level: 2 },
        0x7f7a997: { name: 'greatkatana', level: 3 },
        0x7f7a998: { name: 'greatkatana', level: 4 },

        0x7f7a9a5: { name: 'battleaxe', level: 1 },
        0x7f7a9a6: { name: 'battleaxe', level: 2 },
        0x7f7a9a7: { name: 'battleaxe', level: 3 },
        0x7f7a9a8: { name: 'battleaxe', level: 4 },

        0x7f7a999: { name: 'warhammer', level: 1 },
        0x7f7a99a: { name: 'warhammer', level: 2 },
        0x7f7a99b: { name: 'warhammer', level: 3 },
        0x7f7a99c: { name: 'warhammer', level: 4 },

        0x7f7a9a9: { name: 'staff', level: 1 },
        0x7f7a9aa: { name: 'staff', level: 2 },
        0x7f7a9ab: { name: 'staff', level: 3 },
        0x7f7a9ac: { name: 'staff', level: 4 },

        0x7f7a99d: { name: 'pike', level: 1 },
        0x7f7a99e: { name: 'pike', level: 2 },
        0x7f7a99f: { name: 'pike', level: 3 },
        0x7f7a9a0: { name: 'pike', level: 4 },

        0x7f7a9ad: { name: 'halberd', level: 1 },
        0x7f7a9ae: { name: 'halberd', level: 2 },
        0x7f7a9af: { name: 'halberd', level: 3 },
        0x7f7a9b0: { name: 'halberd', level: 4 },

        0x7f7fab1: { name: 'armorlight', level: 1 },
        0x7f7fab2: { name: 'armorlight', level: 2 },

        0x7f7fab3: { name: 'armorheavy', level: 1 },
        0x7f7fab4: { name: 'armorheavy', level: 2 },

        0x7f7fab5: { name: 'shield', level: 1 },
        0x7f7fab6: { name: 'shield', level: 2 },
        0x7f7fab7: { name: 'shield', level: 3 },
        0x7f7fab8: { name: 'shield', level: 4 },

        0x7f7fac5: { name: 'conjuration', level: 1 },
        0x7f7fac6: { name: 'conjuration', level: 2 },
        0x7f7fac7: { name: 'conjuration', level: 3 },
        0x7f7fac8: { name: 'conjuration', level: 4 },

        0x7f7fab9: { name: 'alchemy', level: 1 },
        0x7f7faba: { name: 'alchemy', level: 2 },
        0x7f7fabb: { name: 'alchemy', level: 3 },
        0x7f7fabc: { name: 'alchemy', level: 4 },

        0x7f7fac9: { name: 'enchant', level: 1 },
        0x7f7faca: { name: 'enchant', level: 2 },
        0x7f7facb: { name: 'enchant', level: 3 },
        0x7f7facc: { name: 'enchant', level: 4 },

        0x7f7fabd: { name: 'alteration', level: 1 },
        0x7f7fabe: { name: 'alteration', level: 2 },
        0x7f7fabf: { name: 'alteration', level: 3 },
        0x7f7fac0: { name: 'alteration', level: 4 },

        0x7f7facd: { name: 'destruction', level: 1 },
        0x7f7face: { name: 'destruction', level: 2 },
        0x7f7facf: { name: 'destruction', level: 3 },
        0x7f7fad0: { name: 'destruction', level: 4 },

        0x7f7fac1: { name: 'illusion', level: 1 },
        0x7f7fac2: { name: 'illusion', level: 2 },
        0x7f7fac3: { name: 'illusion', level: 3 },
        0x7f7fac4: { name: 'illusion', level: 4 },

        0x7f7fad1: { name: 'restoration', level: 1 },
        0x7f7fad2: { name: 'restoration', level: 2 },
        0x7f7fad3: { name: 'restoration', level: 3 },
        0x7f7fad4: { name: 'restoration', level: 4 },
    } as IItemsIds;
    const expId = 0x7f33922;
    const memId = 0x700DE02;
    on('activate', (event) => {
        const altars =
            [
                0x00100780, 0x000D9883, 0x000D987B, 0x000D9881, 0x000D9887, 0x000FB997,
                0x000D9885, 0x000D987D, 0x000071854
            ];
        if (!altars.includes(event.target.getBaseObject()?.getFormID() ||
            -1)) return;
        const player = Game.getPlayer();
        if (!player)
            return;
        const { entries } = getInventory(player);
        const frontData = { exp: 0, mem: 0, perks: {} } as IFrontData;
        entries.forEach(item => {
            if (item.baseId in itemsIds) {
                const itemData = itemsIds[item.baseId];
                frontData.perks[itemData.name] = itemData.level;
            }
            if (item.baseId == expId) {
                frontData.exp = item.count;
            }
            if (item.baseId == memId) {
                frontData.mem = item.count;
            }
        })

        const src = `
            window.dispatchEvent(new CustomEvent('updateSkillMenu', { detail: ${JSON.stringify(frontData)}}))
        `
        browser.setFocused(true)
        browser.executeJavaScript(src)
    })
}
