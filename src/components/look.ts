import { Actor, ActorBase, Game, TESModPlatform, Race, HeadPart, TextureSet, printConsole, VoiceType } from "skyrimPlatform";

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
    name: string;
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
        skinColor: skinColor ? skinColor.getColor() : 0,
        name: actor.getBaseObject().getName()
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
        let tint: Tint = {
            texturePath: Game.getNthTintMaskTexturePath(i),
            type: Game.getNthTintMaskType(i),
            argb: Game.getNthTintMaskColor(i)
        };
        newLook.tints.push(tint);
    }

    return newLook;
};

let isVisible = (argb: number) => argb > 0x00ffffff || argb < 0;

let gFixing = false;

export let applyTints = (actor: Actor, look: Look) => {
    if (!look) throw new Error(`null look has been passed to applyTints`);

    let tints = look.tints.filter(t => isVisible(t.argb));

    let raceWarPaintRegex = /.*Head.+WarPaint.*/;
    let uniWarPaintRegex = /.*HeadWarPaint.*/;
    let raceSpecificWarPaint = tints.filter(t => isVisible(t.argb) && t.texturePath.match(raceWarPaintRegex)).length; // MaleHeadNordWarPaint
    let uniWarPaint = tints.filter(t => isVisible(t.argb) && t.texturePath.match(uniWarPaintRegex)).length; // MaleHeadWarPaint

    if (raceSpecificWarPaint + uniWarPaint > 1) {
        // If visible war paints of these two types present, then Skyrim crashes
        printConsole('bad warpaint!', raceSpecificWarPaint, uniWarPaint);
        return;
    }

    TESModPlatform.clearTintMasks(actor);
    tints.forEach((tint, i) => {
        TESModPlatform.pushTintMask(actor,tint.type, tint.argb, tint.texturePath);
    });
    
    let playerBaseId = Game.getPlayer().getBaseObject().getFormID();

    TESModPlatform.setFormIdUnsafe(actor.getBaseObject(), playerBaseId);
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
    if (look.name) {
        npc.setName(look.name);
    }
    else { // for undefined or empty name
        npc.setName(' ');
    }

    return npc;
}