import * as sp from "skyrimPlatform";

export const init = (): void => {
  addStdPerks();
}

const addStdPerks = (): void => {
  sp.once("update", () => {
    const player = sp.Game.getPlayer()!;
    // Stealth
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xBE126)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xC07C6)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xC07C7)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xC07C8)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xC07C9)));

    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0x58213)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0x105F24)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0x58214)));
    // Magic
    // Warrior
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xCB40D)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xCB40F)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xCB414)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xCB411)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xCB412)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xCB410)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xCB40E)));
  });
}

export const addPerk = (actorIds: number[], perkIds: number[]): void => {
  sp.once("update", () => {
    const actors = actorIds.map(actorId => sp.Actor.from(sp.Game.getFormEx(actorId))).filter(actor => actor) as sp.Actor[];
    const perks = perkIds.map(perkId => sp.Perk.from(sp.Game.getFormEx(perkId))).filter(perk => perk) as sp.Perk[];

    actors.forEach(actor => perks.forEach(perk => actor.addPerk(perk)));
  });
}
