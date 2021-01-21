import * as skyrimPlatform from './skyrimPlatform';

export interface Spell {
	from: (form: skyrimPlatform.Form) => Spell;
	cast: (akSource: skyrimPlatform.ObjectReference, akTarget: skyrimPlatform.ObjectReference) => Promise<void>;
	getCastTime: () => number;
	getCostliestEffectIndex: () => number;
	getEffectAreas: () => number[];
	getEffectDurations: () => number[];
	getEffectMagnitudes: () => number[];
	getEffectiveMagickaCost: (caster: skyrimPlatform.Actor) => number;
	getEquipType: () => skyrimPlatform.EquipSlot;
	getMagicEffects: () => object[];
	getMagickaCost: () => number;
	getNthEffectArea: (index: number) => number;
	getNthEffectDuration: (index: number) => number;
	getNthEffectMagicEffect: (index: number) => skyrimPlatform.MagicEffect;
	getNthEffectMagnitude: (index: number) => number;
	getNumEffects: () => number;
	getPerk: () => skyrimPlatform.Perk;
	isHostile: () => boolean;
	preload: () => void;
	remoteCast: (
		akSource: skyrimPlatform.ObjectReference,
		akBlameActor: skyrimPlatform.Actor,
		akTarget: skyrimPlatform.ObjectReference
	) => Promise<void>;
	setEquipType: (type: skyrimPlatform.EquipSlot) => void;
	setNthEffectArea: (index: number, value: number) => void;
	setNthEffectDuration: (index: number, value: number) => void;
	setNthEffectMagnitude: (index: number, value: number) => void;
	unload: () => void;
}
