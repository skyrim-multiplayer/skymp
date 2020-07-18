import { Actor, ActorBase, Game, TESModPlatform, Race, HeadPart, TextureSet, printConsole, VoiceType, Utility } from "skyrimPlatform";

export interface Tint {
    texturePath: string;
    argb: number;
    type: number;
};

export interface Look {
    isFemale: boolean;
    raceId: number;
    weight: number;
    skinColor: number;
    hairColor: number;
    headpartIds: number[];
    headTextureSetId: number;
    options: number[];
    presets: number[];
    tints: Tint[];
};

export let getLook = (actor: Actor) => {
    let base = ActorBase.from(actor.getBaseObject());

    let hairColor = base.getHairColor();
    let skinColor = TESModPlatform.getSkinColor(base);

    let newLook: Look;
    newLook = {
        isFemale: base.getSex() === 1,
        raceId: base.getRace() ? base.getRace().getFormID() : 0,
        weight: base.getWeight(),
        hairColor: hairColor ? hairColor.getColor() : 0,
        headpartIds: [],
        headTextureSetId: base.getFaceTextureSet() ? base.getFaceTextureSet().getFormID() : 0,
        options: new Array(19),
        presets: new Array(4),
        tints: [],
        skinColor: skinColor ? skinColor.getColor() : 0
    };

    let numHeadparts = base.getNumHeadParts();
    for (let i = 0; i < numHeadparts; ++i) {
        let part = base.getNthHeadPart(i);
        if (part) newLook.headpartIds.push(part.getFormID());
    }

    for (let i = 0; i < newLook.options.length; ++i) {
        newLook.options[i] = base.getFaceMorph(i);
    }

    for (let i = 0; i < newLook.presets.length; ++i) {
        newLook.presets[i] = base.getFacePreset(i);
    }

    let numTints = Game.getPlayer().getFormID() === actor.getFormID() ? Game.getNumTintMasks() : 0;
    for (let i = 0; i < numTints; ++i) {
        //printConsole('ALPHA', TESModPlatform.getNthTintMaskAlpha(i));
        let tint: Tint = {
            texturePath: Game.getNthTintMaskTexturePath(i),
            type: Game.getNthTintMaskType(i),
            argb: Game.getNthTintMaskColor(i)
        };
        newLook.tints.push(tint);
    }

    printConsole('skin',newLook.skinColor);
    printConsole('hair',newLook.hairColor);

    return newLook;
};

export let applyTints = (actor: Actor, look: Look) => {
    /*TESModPlatform.resizeTintsArray(look.tints.length);
    look.tints.forEach((tint, i) => {
        Game.setNthTintMaskColor(i, tint.argb);
        Game.setNthTintMaskTexturePath(tint.texturePath, i);
    });
    //TESModPlatform.setFormIdUnsafe(actor, Game.getPlayer().getBaseObject().getFormID());
    actor.queueNiNodeUpdate();*/

    //Utility.wait(1).then(() => {
    //let item = Game.getFormEx(0x00061CC1);
    //Game.getPlayer().equipItem(item, true, true);
    //printConsole('apply', item);
    //});
};

export let applyLook = (look: Look): ActorBase => {
    let race = Race.from(Game.getFormEx(look.raceId));
    let headparts = look.headpartIds.map(id => HeadPart.from(Game.getFormEx(id))).filter(headpart => !!headpart);

    let npc: ActorBase = TESModPlatform.createNpc();
    if (!npc) throw new Error('createNpc returned null');
    TESModPlatform.setNpcSex(npc, look.isFemale ? 1 : 0);
    if (race) TESModPlatform.setNpcRace(npc, race);
    npc.setWeight(look.weight);
    TESModPlatform.setNpcSkinColor(npc, look.skinColor);
    TESModPlatform.setNpcHairColor(npc, look.hairColor);
    TESModPlatform.resizeHeadpartsArray(npc, headparts.length);
    headparts.forEach((v, i) => npc.setNthHeadPart(v, i));
    npc.setFaceTextureSet(TextureSet.from(Game.getFormEx(look.headTextureSetId))); // setFaceTextureSet supports null argument
    npc.setVoiceType(VoiceType.from(Game.getFormEx(0x0002F7C3)));
    look.options.forEach((v, i) => npc.setFaceMorph(v, i));
    look.presets.forEach((v, i) => npc.setFacePreset(v, i));

    return npc;
}