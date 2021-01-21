import * as skyrimPlatform from './skyrimPlatform';
import { Actor } from './Actor';
import { Debug } from './Debug';
import { Game } from './Game';
import { Input } from './Input';
import { Spell } from './Spell';
import { Utility } from './Utility';
import { Sound } from './Sound';

export type SkyrimEventName =
	| 'update'
	| 'activate'
	| 'waitStop'
	| 'objectLoaded'
	| 'moveAttachDetach'
	| 'lockChanged'
	| 'grabRelease'
	| 'cellFullyLoaded'
	| 'switchRaceComplete'
	| 'uniqueIdChange'
	| 'trackedStats'
	| 'scriptInit'
	| 'reset'
	| 'combatState'
	| 'loadGame'
	| 'deathEnd'
	| 'deathStart'
	| 'containerChanged'
	| 'hit'
	| 'unequip'
	| 'equip'
	| 'magicEffectApply'
	| 'effectFinish'
	| 'effectStart';

export type SkyrimEvent =
	| skyrimPlatform.ActivateEvent
	| skyrimPlatform.WaitStopEvent
	| skyrimPlatform.ObjectLoadedEvent
	| skyrimPlatform.MoveAttachDetachEvent
	| skyrimPlatform.LockChangedEvent
	| skyrimPlatform.GrabReleaseEvent
	| skyrimPlatform.CellFullyLoadedEvent
	| skyrimPlatform.SwitchRaceCompleteEvent
	| skyrimPlatform.UniqueIDChangeEvent
	| skyrimPlatform.TrackedStatsEvent
	| skyrimPlatform.InitScriptEvent
	| skyrimPlatform.ResetEvent
	| skyrimPlatform.CombatEvent
	| skyrimPlatform.DeathEvent
	| skyrimPlatform.ContainerChangedEvent
	| skyrimPlatform.HitEvent
	| skyrimPlatform.EquipEvent
	| skyrimPlatform.MagicEffectApplyEvent
	| skyrimPlatform.ActiveEffectApplyRemoveEvent;

export interface SkyrimPlatform {
	printConsole: (...args: any[]) => void;
	writeScript: (scriptName: string, src: string) => void;
	callNative: (
		className: string,
		functionName: string,
		self?: object,
		...args: any
	) => any;
	getJsMemoryUsage: () => number;
	getPluginSourceCode: (pluginName: string) => string;
	writePlugin: (pluginName: string, newSources: string) => string;
	getPlatformVersion: () => string;
	storage: any;
	settings: any;
	Face: skyrimPlatform.Face;
	ChangeFormNpc: skyrimPlatform.ChangeFormNpc;
	loadGame: (
		pos: number[],
		angle: number[],
		worldOrCell: number,
		changeFormNpc?: skyrimPlatform.ChangeFormNpc
	) => void;
	worldPointToScreenPoint: (...args: number[][]) => number[][];
	PacketType: skyrimPlatform.PacketType;
	Browser: skyrimPlatform.Browser;
	ExtraData: skyrimPlatform.ExtraData;
	ExtraHealth: skyrimPlatform.ExtraHealth;
	ExtraCount: skyrimPlatform.ExtraCount;
	ExtraEnchantment: skyrimPlatform.ExtraEnchantment;
	ExtraCharge: skyrimPlatform.ExtraCharge;
	ExtraTextDisplayData: skyrimPlatform.ExtraTextDisplayData;
	ExtraSoul: skyrimPlatform.ExtraSoul;
	ExtraPoison: skyrimPlatform.ExtraPoison;
	ExtraWorn: skyrimPlatform.ExtraWorn;
	ExtraWornLeft: skyrimPlatform.ExtraWornLeft;
	BaseExtraList: skyrimPlatform.BaseExtraList;
	InventoryChangesEntry: skyrimPlatform.InventoryChangesEntry;
	getExtraContainerChanges: (
		objectReferenceId: number
	) => skyrimPlatform.InventoryChangesEntry[];
	InventoryEntry: skyrimPlatform.InventoryEntry;
	getContainer: (baseId: number) => skyrimPlatform.InventoryEntry[];
	ActivateEvent: skyrimPlatform.ActivateEvent;
	MoveAttachDetachEvent: skyrimPlatform.MoveAttachDetachEvent;
	WaitStopEvent: skyrimPlatform.WaitStopEvent;
	ObjectLoadedEvent: skyrimPlatform.ObjectLoadedEvent;
	LockChangedEvent: skyrimPlatform.LockChangedEvent;
	CellFullyLoadedEvent: skyrimPlatform.CellFullyLoadedEvent;
	GrabReleaseEvent: skyrimPlatform.GrabReleaseEvent;
	SwitchRaceCompleteEvent: skyrimPlatform.SwitchRaceCompleteEvent;
	UniqueIDChangeEvent: skyrimPlatform.UniqueIDChangeEvent;
	TrackedStatsEvent: skyrimPlatform.TrackedStatsEvent;
	InitScriptEvent: skyrimPlatform.InitScriptEvent;
	ResetEvent: skyrimPlatform.ResetEvent;
	CombatEvent: skyrimPlatform.CombatEvent;
	DeathEvent: skyrimPlatform.DeathEvent;
	ContainerChangedEvent: skyrimPlatform.ContainerChangedEvent;
	HitEvent: skyrimPlatform.HitEvent;
	EquipEvent: skyrimPlatform.EquipEvent;
	ActiveEffectApplyRemoveEvent: skyrimPlatform.ActiveEffectApplyRemoveEvent;
	MagicEffectApplyEvent: skyrimPlatform.MagicEffectApplyEvent;

	on: (
		eventName: SkyrimEventName,
		callback: (event?: SkyrimEvent) => void
	) => void;
	once: (
		eventName: SkyrimEventName,
		callback: (event?: SkyrimEvent) => void
	) => void;

	Hooks: skyrimPlatform.Hooks;
	HttpResponse: skyrimPlatform.HttpResponse;
	HttpClient: skyrimPlatform.HttpClient;
	Form: skyrimPlatform.Form;
	Action: skyrimPlatform.Action;
	Activator: skyrimPlatform.Activator;
	ActiveMagicEffect: skyrimPlatform.ActiveMagicEffect;
	ObjectReference: skyrimPlatform.ObjectReference;
	Actor: Actor;
	ActorBase: skyrimPlatform.ActorBase;
	ActorValueInfo: skyrimPlatform.ActorValueInfo;
	Alias: skyrimPlatform.Alias;
	Ammo: skyrimPlatform.Ammo;
	MiscObject: skyrimPlatform.MiscObject;
	Apparatus: skyrimPlatform.Apparatus;
	Armor: skyrimPlatform.Armor;
	ArmorAddon: skyrimPlatform.ArmorAddon;
	Art: skyrimPlatform.Art;
	AssociationType: skyrimPlatform.AssociationType;
	Book: skyrimPlatform.Book;
	Cell: skyrimPlatform.Cell;
	Class: skyrimPlatform.Class;
	ColorForm: skyrimPlatform.ColorForm;
	CombatStyle: skyrimPlatform.CombatStyle;
	ConstructibleObject: skyrimPlatform.ConstructibleObject;
	Container: skyrimPlatform.Container;
	Debug: Debug;
	DefaultObjectManager: skyrimPlatform.DefaultObjectManager;
	Door: skyrimPlatform.Door;
	EffectShader: skyrimPlatform.EffectShader;
	Enchantment: skyrimPlatform.Enchantment;
	EncounterZone: skyrimPlatform.EncounterZone;
	EquipSlot: skyrimPlatform.EquipSlot;
	Explosion: skyrimPlatform.Explosion;
	Faction: skyrimPlatform.Faction;
	Flora: skyrimPlatform.Flora;
	FormList: skyrimPlatform.FormList;
	Furniture: skyrimPlatform.Furniture;
	Game: Game;
	GlobalVariable: skyrimPlatform.GlobalVariable;
	Hazard: skyrimPlatform.Hazard;
	HeadPart: skyrimPlatform.HeadPart;
	Idle: skyrimPlatform.Idle;
	ImageSpaceModifier: skyrimPlatform.ImageSpaceModifier;
	ImpactDataSet: skyrimPlatform.ImpactDataSet;
	Ingredient: skyrimPlatform.Ingredient;
	Input: Input;
	Key: skyrimPlatform.Key;
	Keyword: skyrimPlatform.Keyword;
	LeveledActor: skyrimPlatform.LeveledActor;
	LeveledItem: skyrimPlatform.LeveledItem;
	LeveledSpell: skyrimPlatform.LeveledSpell;
	Light: skyrimPlatform.Light;
	Location: skyrimPlatform.Location;
	LocationAlias: skyrimPlatform.LocationAlias;
	LocationRefType: skyrimPlatform.LocationRefType;
	MagicEffect: skyrimPlatform.MagicEffect;
	Message: skyrimPlatform.Message;
	MusicType: skyrimPlatform.MusicType;
	NetImmerse: skyrimPlatform.NetImmerse;
	Outfit: skyrimPlatform.Outfit;
	Projectile: skyrimPlatform.Projectile;
	Package: skyrimPlatform.Package;
	Perk: skyrimPlatform.Perk;
	Potion: skyrimPlatform.Potion;
	Quest: skyrimPlatform.Quest;
	Race: skyrimPlatform.Race;
	ReferenceAlias: skyrimPlatform.ReferenceAlias;
	Spell: Spell;
	Static: skyrimPlatform.Static;
	Scene: skyrimPlatform.Scene;
	Scroll: skyrimPlatform.Scroll;
	ShaderParticleGeometry: skyrimPlatform.ShaderParticleGeometry;
	Shout: skyrimPlatform.Shout;
	SoulGem: skyrimPlatform.SoulGem;
	Sound: Sound;
	SoundCategory: skyrimPlatform.SoundCategory;
	SoundDescriptor: skyrimPlatform.SoundDescriptor;
	TESModPlatform: skyrimPlatform.TESModPlatform;
	TalkingActivator: skyrimPlatform.TalkingActivator;
	TextureSet: skyrimPlatform.TextureSet;
	Topic: skyrimPlatform.Topic;
	TopicInfo: skyrimPlatform.TopicInfo;
	TreeObject: skyrimPlatform.TreeObject;
	Ui: skyrimPlatform.Ui;
	VisualEffect: skyrimPlatform.VisualEffect;
	VoiceType: skyrimPlatform.VoiceType;
	Weapon: skyrimPlatform.Weapon;
	Weather: skyrimPlatform.Weather;
	WordOfPower: skyrimPlatform.WordOfPower;
	WorldSpace: skyrimPlatform.WorldSpace;
	Utility: Utility;
}
