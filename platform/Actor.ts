import * as skyrimPlatform from './skyrimPlatform';

export interface Actor {
	from: (form: skyrimPlatform.Form) => Actor;
	addPerk: (akPerk: skyrimPlatform.Perk) => void;
	addShout: (akShout: skyrimPlatform.Shout) => boolean;
	addSpell: (akSpell: skyrimPlatform.Spell, abVerbose: boolean) => boolean;
	allowBleedoutDialogue: (abCanTalk: boolean) => void;
	allowPCDialogue: (abTalk: boolean) => void;
	attachAshPile: (akAshPileBase: skyrimPlatform.Form) => void;
	canFlyHere: () => boolean;
	changeHeadPart: (hPart: skyrimPlatform.HeadPart) => void;
	clearArrested: () => void;
	clearExpressionOverride: () => void;
	clearExtraArrows: () => void;
	clearForcedMovement: () => void;
	clearKeepOffsetFromActor: () => void;
	clearLookAt: () => void;
	damageActorValue: (asValueName: string, afDamage: number) => void;
	dismount: () => boolean;
	dispelAllSpells: () => void;
	dispelSpell: (akSpell: skyrimPlatform.Spell) => boolean;
	doCombatSpellApply: (akSpell: skyrimPlatform.Spell, akTarget: skyrimPlatform.ObjectReference) => void;
	drawWeapon: () => void;
	enableAI: (abEnable: boolean) => void;
	endDeferredKill: () => void;
	equipItem: (akItem: skyrimPlatform.Form, abPreventRemoval: boolean, abSilent: boolean) => void;
	equipItemById: (
		item: skyrimPlatform.Form,
		itemId: number,
		equipSlot: number,
		preventUnequip: boolean,
		equipSound: boolean
	) => void;
	equipItemEx: (item: skyrimPlatform.Form, equipSlot: number, preventUnequip: boolean, equipSound: boolean) => void;
	equipShout: (akShout: skyrimPlatform.Shout) => void;
	equipSpell: (akSpell: skyrimPlatform.Spell, aiSource: number) => void;
	evaluatePackage: () => void;
	forceActorValue: (asValueName: string, afNewValue: number) => void;
	forceMovementDirection: (afXAngle: number, afYAngle: number, afZAngle: number) => void;
	forceMovementDirectionRamp: (afXAngle: number, afYAngle: number, afZAngle: number, afRampTime: number) => void;
	forceMovementRotationSpeed: (afXMult: number, afYMult: number, afZMult: number) => void;
	forceMovementRotationSpeedRamp: (afXMult: number, afYMult: number, afZMult: number, afRampTime: number) => void;
	forceMovementSpeed: (afSpeedMult: number) => void;
	forceMovementSpeedRamp: (afSpeedMult: number, afRampTime: number) => void;
	forceTargetAngle: (afXAngle: number, afYAngle: number, afZAngle: number) => void;
	forceTargetDirection: (afXAngle: number, afYAngle: number, afZAngle: number) => void;
	forceTargetSpeed: (afSpeed: number) => void;
	getActorValue: (asValueName: string) => number;
	getActorValueMax: (asValueName: string) => number;
	getActorValuePercentage: (asValueName: string) => number;
	getBaseActorValue: (asValueName: string) => number;
	getBribeAmount: () => number;
	getCombatState: () => number;
	getCombatTarget: () => skyrimPlatform.Actor;
	getCrimeFaction: () => skyrimPlatform.Faction;
	getCurrentPackage: () => skyrimPlatform.Package;
	getDialogueTarget: () => skyrimPlatform.Actor;
	getEquippedArmorInSlot: (aiSlot: number) => skyrimPlatform.Armor;
	getEquippedItemId: (Location: number) => number;
	getEquippedItemType: (aiHand: number) => number;
	getEquippedObject: (Location: number) => skyrimPlatform.Form;
	getEquippedShield: () => skyrimPlatform.Armor;
	getEquippedShout: () => skyrimPlatform.Shout;
	getEquippedSpell: (aiSource: number) => skyrimPlatform.Spell;
	getEquippedWeapon: (abLeftHand: boolean) => skyrimPlatform.Weapon;
	getFactionRank: (akFaction: skyrimPlatform.Faction) => number;
	getFactionReaction: (akOther: skyrimPlatform.Actor) => number;
	getFactions: (minRank: number, maxRank: number) => object[];
	getFlyingState: () => number;
	getForcedLandingMarker: () => skyrimPlatform.ObjectReference;
	getFurnitureReference: () => skyrimPlatform.ObjectReference;
	getGoldAmount: () => number;
	getHighestRelationshipRank: () => number;
	getKiller: () => skyrimPlatform.Actor;
	getLevel: () => number;
	getLeveledActorBase: () => skyrimPlatform.ActorBase;
	getLightLevel: () => number;
	getLowestRelationshipRank: () => number;
	getNoBleedoutRecovery: () => boolean;
	getNthSpell: (n: number) => skyrimPlatform.Spell;
	getPlayerControls: () => boolean;
	getRace: () => skyrimPlatform.Race;
	getRelationshipRank: (akOther: skyrimPlatform.Actor) => number;
	getSitState: () => number;
	getSleepState: () => number;
	getSpellCount: () => number;
	getVoiceRecoveryTime: () => number;
	getWarmthRating: () => number;
	getWornForm: (slotMask: number) => skyrimPlatform.Form;
	getWornItemId: (slotMask: number) => number;
	hasAssociation: (akAssociation: skyrimPlatform.AssociationType, akOther: skyrimPlatform.Actor) => boolean;
	hasFamilyRelationship: (akOther: skyrimPlatform.Actor) => boolean;
	hasLOS: (akOther: skyrimPlatform.ObjectReference) => boolean;
	hasMagicEffect: (akEffect: skyrimPlatform.MagicEffect) => boolean;
	hasMagicEffectWithKeyword: (akKeyword: skyrimPlatform.Keyword) => boolean;
	hasParentRelationship: (akOther: skyrimPlatform.Actor) => boolean;
	hasPerk: (akPerk: skyrimPlatform.Perk) => boolean;
	hasSpell: (akForm: skyrimPlatform.Form) => boolean;
	isAIEnabled: () => boolean;
	isAlarmed: () => boolean;
	isAlerted: () => boolean;
	isAllowedToFly: () => boolean;
	isArrested: () => boolean;
	isArrestingTarget: () => boolean;
	isBeingRidden: () => boolean;
	isBleedingOut: () => boolean;
	isBribed: () => boolean;
	isChild: () => boolean;
	isCommandedActor: () => boolean;
	isDead: () => boolean;
	isDetectedBy: (akOther: skyrimPlatform.Actor) => boolean;
	isDoingFavor: () => boolean;
	isEquipped: (akItem: skyrimPlatform.Form) => boolean;
	isEssential: () => boolean;
	isFlying: () => boolean;
	isGhost: () => boolean;
	isGuard: () => boolean;
	isHostileToActor: (akActor: skyrimPlatform.Actor) => boolean;
	isInCombat: () => boolean;
	isInFaction: (akFaction: skyrimPlatform.Faction) => boolean;
	isInKillMove: () => boolean;
	isIntimidated: () => boolean;
	isOnMount: () => boolean;
	isOverEncumbered: () => boolean;
	isPlayerTeammate: () => boolean;
	isPlayersLastRiddenHorse: () => boolean;
	isRunning: () => boolean;
	isSneaking: () => boolean;
	isSprinting: () => boolean;
	isSwimming: () => boolean;
	isTrespassing: () => boolean;
	isUnconscious: () => boolean;
	isWeaponDrawn: () => boolean;
	keepOffsetFromActor: (
		arTarget: skyrimPlatform.Actor,
		afOffsetX: number,
		afOffsetY: number,
		afOffsetZ: number,
		afOffsetAngleX: number,
		afOffsetAngleY: number,
		afOffsetAngleZ: number,
		afCatchUpRadius: number,
		afFollowRadius: number
	) => void;
	kill: (akKiller: skyrimPlatform.Actor) => void;
	killSilent: (akKiller: skyrimPlatform.Actor) => void;
	modActorValue: (asValueName: string, afAmount: number) => void;
	modFactionRank: (akFaction: skyrimPlatform.Faction, aiMod: number) => void;
	moveToPackageLocation: () => Promise<void>;
	openInventory: (abForceOpen: boolean) => void;
	pathToReference: (aTarget: skyrimPlatform.ObjectReference, afWalkRunPercent: number) => Promise<boolean>;
	playIdle: (akIdle: skyrimPlatform.Idle) => boolean;
	playIdleWithTarget: (akIdle: skyrimPlatform.Idle, akTarget: skyrimPlatform.ObjectReference) => boolean;
	playSubGraphAnimation: (asEventName: string) => void;
	queueNiNodeUpdate: () => void;
	regenerateHead: () => void;
	removeFromAllFactions: () => void;
	removeFromFaction: (akFaction: skyrimPlatform.Faction) => void;
	removePerk: (akPerk: skyrimPlatform.Perk) => void;
	removeShout: (akShout: skyrimPlatform.Shout) => boolean;
	removeSpell: (akSpell: skyrimPlatform.Spell) => boolean;
	replaceHeadPart: (oPart: skyrimPlatform.HeadPart, newPart: skyrimPlatform.HeadPart) => void;
	resetAI: () => void;
	resetExpressionOverrides: () => void;
	resetHealthAndLimbs: () => void;
	restoreActorValue: (asValueName: string, afAmount: number) => void;
	resurrect: () => Promise<void>;
	sendAssaultAlarm: () => void;
	sendLycanthropyStateChanged: (abIsWerewolf: boolean) => void;
	sendTrespassAlarm: (akCriminal: skyrimPlatform.Actor) => void;
	sendVampirismStateChanged: (abIsVampire: boolean) => void;
	setActorValue: (asValueName: string, afValue: number) => void;
	setAlert: (abAlerted: boolean) => void;
	setAllowFlying: (abAllowed: boolean) => void;
	setAllowFlyingEx: (abAllowed: boolean, abAllowCrash: boolean, abAllowSearch: boolean) => void;
	setAlpha: (afTargetAlpha: number, abFade: boolean) => void;
	setAttackActorOnSight: (abAttackOnSight: boolean) => void;
	setBribed: (abBribe: boolean) => void;
	setCrimeFaction: (akFaction: skyrimPlatform.Faction) => void;
	setCriticalStage: (aiStage: number) => void;
	setDoingFavor: (abDoingFavor: boolean) => void;
	setDontMove: (abDontMove: boolean) => void;
	setExpressionModifier: (index: number, value: number) => void;
	setExpressionOverride: (aiMood: number, aiStrength: number) => void;
	setExpressionPhoneme: (index: number, value: number) => void;
	setEyeTexture: (akNewTexture: skyrimPlatform.TextureSet) => void;
	setFactionRank: (akFaction: skyrimPlatform.Faction, aiRank: number) => void;
	setForcedLandingMarker: (aMarker: skyrimPlatform.ObjectReference) => void;
	setGhost: (abIsGhost: boolean) => void;
	setHeadTracking: (abEnable: boolean) => void;
	setIntimidated: (abIntimidate: boolean) => void;
	setLookAt: (akTarget: skyrimPlatform.ObjectReference, abPathingLookAt: boolean) => void;
	setNoBleedoutRecovery: (abAllowed: boolean) => void;
	setNotShowOnStealthMeter: (abNotShow: boolean) => void;
	setOutfit: (akOutfit: skyrimPlatform.Outfit, abSleepOutfit: boolean) => void;
	setPlayerControls: (abControls: boolean) => void;
	setPlayerResistingArrest: () => void;
	setPlayerTeammate: (abTeammate: boolean, abCanDoFavor: boolean) => void;
	setRace: (akRace: skyrimPlatform.Race) => void;
	setRelationshipRank: (akOther: skyrimPlatform.Actor, aiRank: number) => void;
	setRestrained: (abRestrained: boolean) => void;
	setSubGraphFloatVariable: (asVariableName: string, afValue: number) => void;
	setUnconscious: (abUnconscious: boolean) => void;
	setVehicle: (akVehicle: skyrimPlatform.ObjectReference) => void;
	setVoiceRecoveryTime: (afTime: number) => void;
	sheatheWeapon: () => void;
	showBarterMenu: () => void;
	showGiftMenu: (
		abGivingGift: boolean,
		apFilterList: skyrimPlatform.FormList,
		abShowStolenItems: boolean,
		abUseFavorPoints: boolean
	) => Promise<number>;
	startCannibal: (akTarget: skyrimPlatform.Actor) => void;
	startCombat: (akTarget: skyrimPlatform.Actor) => void;
	startDeferredKill: () => void;
	startSneaking: () => void;
	startVampireFeed: (akTarget: skyrimPlatform.Actor) => void;
	stopCombat: () => void;
	stopCombatAlarm: () => void;
	trapSoul: (akTarget: skyrimPlatform.Actor) => boolean;
	unLockOwnedDoorsInCell: () => void;
	unequipAll: () => void;
	unequipItem: (akItem: skyrimPlatform.Form, abPreventEquip: boolean, abSilent: boolean) => void;
	unequipItemEx: (item: skyrimPlatform.Form, equipSlot: number, preventEquip: boolean) => void;
	unequipItemSlot: (aiSlot: number) => void;
	unequipShout: (akShout: skyrimPlatform.Shout) => void;
	unequipSpell: (akSpell: skyrimPlatform.Spell, aiSource: number) => void;
	updateWeight: (neckDelta: number) => void;
	willIntimidateSucceed: () => boolean;
	wornHasKeyword: (akKeyword: skyrimPlatform.Keyword) => boolean;
}
