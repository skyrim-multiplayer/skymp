import * as sp from 'skyrimPlatform';

export const init = (): void => {
  addStdPerks();
};

const addStdPerks = (): void => {
  sp.once('update', () => {
    const player = sp.Game.getPlayer()!;
    // Stealth
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xbe126)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xc07c6)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xc07c7)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xc07c8)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xc07c9)));

    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0x58213)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0x105f24)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0x58214)));
    // Magic
    // Warrior
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xcb40d)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xcb40f)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xcb414)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xcb411)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xcb412)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xcb410)));
    player.addPerk(sp.Perk.from(sp.Game.getFormEx(0xcb40e)));
  });
};

export const addPerk = (actorIds: number[], perkIds: number[]): void => {
  sp.once('update', () => {
    const actors = actorIds
      .map((actorId) => sp.Actor.from(sp.Game.getFormEx(actorId)))
      .filter((actor) => actor) as sp.Actor[];
    const perks = perkIds
      .map((perkId) => sp.Perk.from(sp.Game.getFormEx(perkId)))
      .filter((perk) => perk) as sp.Perk[];

    actors.forEach((actor) => perks.forEach((perk) => actor.addPerk(perk)));
  });
};
