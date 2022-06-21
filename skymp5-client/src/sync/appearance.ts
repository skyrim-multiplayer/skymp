import {
  Actor,
  ActorBase,
  Game,
  TESModPlatform,
  Race,
  HeadPart,
  TextureSet,
  printConsole,
  VoiceType,
  once,
  Utility,
} from "skyrimPlatform";
import * as deathSystem from "./deathSystem";

export interface Tint {
  texturePath: string;
  argb: number;
  type: number;
}

export interface Appearance {
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
}

export const getAppearance = (actor: Actor): Appearance => {
  const base = ActorBase.from(actor.getBaseObject()) as ActorBase;

  const hairColor = base.getHairColor();
  const skinColor = TESModPlatform.getSkinColor(base);

  const newAppearance: Appearance = {
    isFemale: base.getSex() === 1,
    raceId: base.getRace() ? (base.getRace() as Race).getFormID() : 0,
    weight: base.getWeight(),
    hairColor: hairColor ? hairColor.getColor() : 0,
    headpartIds: [],
    headTextureSetId: base.getFaceTextureSet()
      ? (base.getFaceTextureSet() as TextureSet).getFormID()
      : 0,
    options: new Array(19),
    presets: new Array(4),
    tints: [],
    skinColor: skinColor ? skinColor.getColor() : 0,
    name: (actor.getBaseObject() as ActorBase).getName(),
  };

  const numHeadparts = base.getNumHeadParts();
  for (let i = 0; i < numHeadparts; ++i) {
    const part = base.getNthHeadPart(i);
    if (part) newAppearance.headpartIds.push(part.getFormID());
  }

  for (let i = 0; i < newAppearance.options.length; ++i) {
    newAppearance.options[i] = base.getFaceMorph(i);
  }

  for (let i = 0; i < newAppearance.presets.length; ++i) {
    newAppearance.presets[i] = base.getFacePreset(i);
  }

  const numTints =
    (Game.getPlayer() as Actor).getFormID() === actor.getFormID()
      ? Game.getNumTintMasks()
      : 0;
  for (let i = 0; i < numTints; ++i) {
    const tint: Tint = {
      texturePath: Game.getNthTintMaskTexturePath(i),
      type: Game.getNthTintMaskType(i),
      argb: Game.getNthTintMaskColor(i),
    };
    newAppearance.tints.push(tint);
  }

  return newAppearance;
};

const isVisible = (argb: number) => argb > 0x00ffffff || argb < 0;

export const applyTints = (actor: Actor | null, appearance: Appearance): void => {
  if (!appearance) throw new Error("null appearance has been passed to applyTints");

  const tints = appearance.tints.filter((t) => isVisible(t.argb));

  const raceWarPaintRegex = /.*Head.+WarPaint.*/;
  const uniWarPaintRegex = /.*HeadWarPaint.*/;
  const raceSpecificWarPaint = tints.filter(
    (t) => isVisible(t.argb) && t.texturePath.match(raceWarPaintRegex)
  ).length; // MaleHeadNordWarPaint
  const uniWarPaint = tints.filter(
    (t) => isVisible(t.argb) && t.texturePath.match(uniWarPaintRegex)
  ).length; // MaleHeadWarPaint

  if (raceSpecificWarPaint + uniWarPaint > 1) {
    // If visible war paints of these two types present, then Skyrim crashes
    printConsole("bad warpaint!", raceSpecificWarPaint, uniWarPaint);
    return;
  }

  TESModPlatform.clearTintMasks(actor);
  tints.forEach((tint) => {
    TESModPlatform.pushTintMask(actor, tint.type, tint.argb, tint.texturePath);
  });

  const playerBaseId = ((Game.getPlayer() as Actor).getBaseObject() as ActorBase).getFormID();

  if (actor)
    TESModPlatform.setFormIdUnsafe(actor.getBaseObject(), playerBaseId);
};

export const silentVoiceTypeId = 0x0002f7c3;

const applyAppearanceCommon = (appearance: Appearance, npc: ActorBase): void => {
  const race = Race.from(Game.getFormEx(appearance.raceId));
  const headparts = appearance.headpartIds
    .map((id) => HeadPart.from(Game.getFormEx(id)))
    .filter((headpart) => !!headpart);

  TESModPlatform.setNpcSex(npc, appearance.isFemale ? 1 : 0);
  if (race) TESModPlatform.setNpcRace(npc, race);
  npc.setWeight(appearance.weight);
  TESModPlatform.setNpcSkinColor(npc, appearance.skinColor);
  TESModPlatform.setNpcHairColor(npc, appearance.hairColor);
  TESModPlatform.resizeHeadpartsArray(npc, headparts.length);
  headparts.forEach((v, i) => npc.setNthHeadPart(v, i));
  npc.setFaceTextureSet(TextureSet.from(Game.getFormEx(appearance.headTextureSetId))); // setFaceTextureSet supports null argument
  npc.setVoiceType(VoiceType.from(Game.getFormEx(silentVoiceTypeId)));
  appearance.options.forEach((v, i) => npc.setFaceMorph(v, i));
  appearance.presets.forEach((v, i) => npc.setFacePreset(v, i));
  if (appearance.name) {
    npc.setName(appearance.name);
  } else {
    // for undefined or empty name
    npc.setName(" ");
  }
};

export const applyAppearance = (appearance: Appearance): ActorBase => {
  const npc: ActorBase = TESModPlatform.createNpc() as ActorBase;
  if (!npc) throw new Error("createNpc returned null");
  applyAppearanceCommon(appearance, npc);
  return npc;
};

export const applyAppearanceToPlayer = (appearance: Appearance): void => {
  applyAppearanceCommon(
    appearance,
    ActorBase.from((Game.getPlayer() as Actor).getBaseObject()) as ActorBase
  );
  applyTints(Game.getPlayer() as Actor, appearance);
  (Game.getPlayer() as Actor).queueNiNodeUpdate();
  Utility.wait(0.0625).then(() => {
    once("update", () => {
      deathSystem.makeActorImmortal(Game.getPlayer() as Actor);
    });
  });
};
