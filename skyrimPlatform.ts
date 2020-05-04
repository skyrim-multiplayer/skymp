
// Generated automatically. Do not edit.
export declare function printConsole(...arguments: any[]): void;
export declare function writeScript(scriptName: string, src: string): void;
export declare function on(eventName: string, callback: any): void;
export declare function callNative(className: string, functionName: string, self?: object, ...args: any): any;
export declare function getJsMemoryUsage(): number;
export declare let storage: any;

// Based on ActiveMagicEffect.pex
export declare class ActiveMagicEffect {
    static from(form: Form): ActiveMagicEffect;
    addInventoryEventFilter(akFilter: Form): void;
    dispel(): void;
    getBaseObject(): MagicEffect;
    getCasterActor(): Actor;
    getDuration(): number;
    getMagnitude(): number;
    getTargetActor(): Actor;
    getTimeElapsed(): number;
    registerForActorAction(actionType: number): void;
    registerForAnimationEvent(akSender: ObjectReference, asEventName: string): boolean;
    registerForCameraState(): void;
    registerForControl(control: string): void;
    registerForCrosshairRef(): void;
    registerForKey(keyCode: number): void;
    registerForLOS(akViewer: Actor, akTarget: ObjectReference): void;
    registerForMenu(menuName: string): void;
    registerForModEvent(eventName: string, callbackName: string): void;
    registerForNiNodeUpdate(): void;
    registerForSingleLOSGain(akViewer: Actor, akTarget: ObjectReference): void;
    registerForSingleLOSLost(akViewer: Actor, akTarget: ObjectReference): void;
    registerForSingleUpdate(afInterval: number): void;
    registerForSingleUpdateGameTime(afInterval: number): void;
    registerForSleep(): void;
    registerForTrackedStatsEvent(): void;
    registerForUpdate(afInterval: number): void;
    registerForUpdateGameTime(afInterval: number): void;
    removeAllInventoryEventFilters(): void;
    removeInventoryEventFilter(akFilter: Form): void;
    sendModEvent(eventName: string, strArg: string, numArg: number): void;
    startObjectProfiling(): void;
    stopObjectProfiling(): void;
    unregisterForActorAction(actionType: number): void;
    unregisterForAllControls(): void;
    unregisterForAllKeys(): void;
    unregisterForAllMenus(): void;
    unregisterForAllModEvents(): void;
    unregisterForAnimationEvent(akSender: ObjectReference, asEventName: string): void;
    unregisterForCameraState(): void;
    unregisterForControl(control: string): void;
    unregisterForCrosshairRef(): void;
    unregisterForKey(keyCode: number): void;
    unregisterForLOS(akViewer: Actor, akTarget: ObjectReference): void;
    unregisterForMenu(menuName: string): void;
    unregisterForModEvent(eventName: string): void;
    unregisterForNiNodeUpdate(): void;
    unregisterForSleep(): void;
    unregisterForTrackedStatsEvent(): void;
    unregisterForUpdate(): void;
    unregisterForUpdateGameTime(): void;
}

// Based on Form.pex
export declare class Form {
    static from(form: Form): Form;
    getFormID(): number;
    getGoldValue(): number;
    getKeywords(): object[];
    getName(): string;
    getNthKeyword(index: number): Keyword;
    getNumKeywords(): number;
    getType(): number;
    getWeight(): number;
    getWorldModelNthTextureSet(n: number): TextureSet;
    getWorldModelNumTextureSets(): number;
    getWorldModelPath(): string;
    hasKeyword(akKeyword: Keyword): boolean;
    hasWorldModel(): boolean;
    isPlayable(): boolean;
    playerKnows(): boolean;
    registerForActorAction(actionType: number): void;
    registerForAnimationEvent(akSender: ObjectReference, asEventName: string): boolean;
    registerForCameraState(): void;
    registerForControl(control: string): void;
    registerForCrosshairRef(): void;
    registerForKey(keyCode: number): void;
    registerForLOS(akViewer: Actor, akTarget: ObjectReference): void;
    registerForMenu(menuName: string): void;
    registerForModEvent(eventName: string, callbackName: string): void;
    registerForNiNodeUpdate(): void;
    registerForSingleLOSGain(akViewer: Actor, akTarget: ObjectReference): void;
    registerForSingleLOSLost(akViewer: Actor, akTarget: ObjectReference): void;
    registerForSingleUpdate(afInterval: number): void;
    registerForSingleUpdateGameTime(afInterval: number): void;
    registerForSleep(): void;
    registerForTrackedStatsEvent(): void;
    registerForUpdate(afInterval: number): void;
    registerForUpdateGameTime(afInterval: number): void;
    sendModEvent(eventName: string, strArg: string, numArg: number): void;
    setGoldValue(value: number): void;
    setName(name: string): void;
    setPlayerKnows(knows: boolean): void;
    setWeight(weight: number): void;
    setWorldModelNthTextureSet(nSet: TextureSet, n: number): void;
    setWorldModelPath(path: string): void;
    startObjectProfiling(): void;
    stopObjectProfiling(): void;
    tempClone(): Form;
    unregisterForActorAction(actionType: number): void;
    unregisterForAllControls(): void;
    unregisterForAllKeys(): void;
    unregisterForAllMenus(): void;
    unregisterForAllModEvents(): void;
    unregisterForAnimationEvent(akSender: ObjectReference, asEventName: string): void;
    unregisterForCameraState(): void;
    unregisterForControl(control: string): void;
    unregisterForCrosshairRef(): void;
    unregisterForKey(keyCode: number): void;
    unregisterForLOS(akViewer: Actor, akTarget: ObjectReference): void;
    unregisterForMenu(menuName: string): void;
    unregisterForModEvent(eventName: string): void;
    unregisterForNiNodeUpdate(): void;
    unregisterForSleep(): void;
    unregisterForTrackedStatsEvent(): void;
    unregisterForUpdate(): void;
    unregisterForUpdateGameTime(): void;
}

// Based on ObjectReference.pex
export declare class ObjectReference extends Form {
    static from(form: Form): ObjectReference;
    activate(akActivator: ObjectReference, abDefaultProcessingOnly: boolean): boolean;
    addDependentAnimatedObjectReference(akDependent: ObjectReference): boolean;
    addInventoryEventFilter(akFilter: Form): void;
    addItem(akItemToAdd: Form, aiCount: number, abSilent: boolean): void;
    addToMap(abAllowFastTravel: boolean): void;
    applyHavokImpulse(afX: number, afY: number, afZ: number, afMagnitude: number): void;
    blockActivation(abBlocked: boolean): void;
    calculateEncounterLevel(aiDifficulty: number): number;
    canFastTravelToMarker(): boolean;
    clearDestruction(): void;
    createDetectionEvent(akOwner: Actor, aiSoundLevel: number): void;
    createEnchantment(maxCharge: number, effects: object[], magnitudes: number[], areas: number[], durations: number[]): void;
    damageObject(afDamage: number): void;
    delete(): void;
    disable(abFadeOut: boolean): void;
    disableNoWait(abFadeOut: boolean): void;
    dropObject(akObject: Form, aiCount: number): ObjectReference;
    enable(abFadeIn: boolean): void;
    enableFastTravel(abEnable: boolean): void;
    enableNoWait(abFadeIn: boolean): void;
    forceAddRagdollToWorld(): void;
    forceRemoveRagdollFromWorld(): void;
    getActorOwner(): ActorBase;
    getAllForms(toFill: FormList): void;
    getAngleX(): number;
    getAngleY(): number;
    getAngleZ(): number;
    getAnimationVariableBool(arVariableName: string): boolean;
    getAnimationVariableFloat(arVariableName: string): number;
    getAnimationVariableInt(arVariableName: string): number;
    getBaseObject(): Form;
    getContainerForms(): object[];
    getCurrentDestructionStage(): number;
    getCurrentLocation(): Location;
    getCurrentScene(): Scene;
    getDisplayName(): string;
    getEditorLocation(): Location;
    getEnableParent(): ObjectReference;
    getEnchantment(): Enchantment;
    getFactionOwner(): Faction;
    getHeadingAngle(akOther: ObjectReference): number;
    getHeight(): number;
    getItemCharge(): number;
    getItemCount(akItem: Form): number;
    getItemHealthPercent(): number;
    getItemMaxCharge(): number;
    getKey(): Form;
    getLength(): number;
    getLinkedRef(apKeyword: Keyword): ObjectReference;
    getLockLevel(): number;
    getMass(): number;
    getNthForm(index: number): Form;
    getNthLinkedRef(aiLinkedRef: number): ObjectReference;
    getNthReferenceAlias(n: number): ReferenceAlias;
    getNumItems(): number;
    getNumReferenceAliases(): number;
    getOpenState(): number;
    getParentCell(): Cell;
    getPoison(): Potion;
    getPositionX(): number;
    getPositionY(): number;
    getPositionZ(): number;
    getReferenceAliases(): object[];
    getScale(): number;
    getTotalArmorWeight(): number;
    getTotalItemWeight(): number;
    getTriggerObjectCount(): number;
    getVoiceType(): Form;
    getWidth(): number;
    getWorldSpace(): Form;
    hasEffectKeyword(akKeyword: Keyword): boolean;
    hasNode(asNodeName: string): boolean;
    hasRefType(akRefType: Form): boolean;
    ignoreFriendlyHits(abIgnore: boolean): void;
    interruptCast(): void;
    is3DLoaded(): boolean;
    isActivateChild(akChild: ObjectReference): boolean;
    isActivationBlocked(): boolean;
    isDeleted(): boolean;
    isDisabled(): boolean;
    isFurnitureInUse(abIgnoreReserved: boolean): boolean;
    isFurnitureMarkerInUse(aiMarker: number, abIgnoreReserved: boolean): boolean;
    isHarvested(): boolean;
    isIgnoringFriendlyHits(): boolean;
    isInDialogueWithPlayer(): boolean;
    isLockBroken(): boolean;
    isLocked(): boolean;
    isMapMarkerVisible(): boolean;
    isOffLimits(): boolean;
    knockAreaEffect(afMagnitude: number, afRadius: number): void;
    lock(abLock: boolean, abAsOwner: boolean): void;
    moveTo(akTarget: ObjectReference, afXOffset: number, afYOffset: number, afZOffset: number, abMatchRotation: boolean): void;
    moveToInteractionLocation(akTarget: ObjectReference): void;
    moveToMyEditorLocation(): void;
    moveToNode(akTarget: ObjectReference, asNodeName: string): void;
    placeActorAtMe(akActorToPlace: ActorBase, aiLevelMod: number, akZone: Form): Actor;
    placeAtMe(akFormToPlace: Form, aiCount: number, abForcePersist: boolean, abInitiallyDisabled: boolean): ObjectReference;
    playAnimation(asAnimation: string): boolean;
    playAnimationAndWait(asAnimation: string, asEventName: string): boolean;
    playGamebryoAnimation(asAnimation: string, abStartOver: boolean, afEaseInTime: number): boolean;
    playImpactEffect(akImpactEffect: Form, asNodeName: string, afPickDirX: number, afPickDirY: number, afPickDirZ: number, afPickLength: number, abApplyNodeRotation: boolean, abUseNodeLocalRotation: boolean): boolean;
    playSyncedAnimationAndWaitSS(asAnimation1: string, asEvent1: string, akObj2: ObjectReference, asAnimation2: string, asEvent2: string): boolean;
    playSyncedAnimationSS(asAnimation1: string, akObj2: ObjectReference, asAnimation2: string): boolean;
    playTerrainEffect(asEffectModelName: string, asAttachBoneName: string): void;
    processTrapHit(akTrap: ObjectReference, afDamage: number, afPushback: number, afXVel: number, afYVel: number, afZVel: number, afXPos: number, afYPos: number, afZPos: number, aeMaterial: number, afStagger: number): void;
    pushActorAway(akActorToPush: Actor, aiKnockbackForce: number): void;
    removeAllInventoryEventFilters(): void;
    removeAllItems(akTransferTo: ObjectReference, abKeepOwnership: boolean, abRemoveQuestItems: boolean): void;
    removeDependentAnimatedObjectReference(akDependent: ObjectReference): boolean;
    removeInventoryEventFilter(akFilter: Form): void;
    removeItem(akItemToRemove: Form, aiCount: number, abSilent: boolean, akOtherContainer: ObjectReference): void;
    reset(akTarget: ObjectReference): void;
    resetInventory(): void;
    say(akTopicToSay: Topic, akActorToSpeakAs: Actor, abSpeakInPlayersHead: boolean): void;
    sendStealAlarm(akThief: Actor): void;
    setActorCause(akActor: Actor): void;
    setActorOwner(akActorBase: ActorBase): void;
    setAngle(afXAngle: number, afYAngle: number, afZAngle: number): void;
    setAnimationVariableBool(arVariableName: string, abNewValue: boolean): void;
    setAnimationVariableFloat(arVariableName: string, afNewValue: number): void;
    setAnimationVariableInt(arVariableName: string, aiNewValue: number): void;
    setDestroyed(abDestroyed: boolean): void;
    setDisplayName(name: string, force: boolean): boolean;
    setEnchantment(source: Enchantment, maxCharge: number): void;
    setFactionOwner(akFaction: Faction): void;
    setHarvested(harvested: boolean): void;
    setItemCharge(charge: number): void;
    setItemHealthPercent(health: number): void;
    setItemMaxCharge(maxCharge: number): void;
    setLockLevel(aiLockLevel: number): void;
    setMotionType(aeMotionType: number, abAllowActivate: boolean): void;
    setNoFavorAllowed(abNoFavor: boolean): void;
    setOpen(abOpen: boolean): void;
    setPosition(afX: number, afY: number, afZ: number): void;
    setScale(afScale: number): void;
    splineTranslateTo(afX: number, afY: number, afZ: number, afXAngle: number, afYAngle: number, afZAngle: number, afTangentMagnitude: number, afSpeed: number, afMaxRotationSpeed: number): void;
    splineTranslateToRefNode(arTarget: ObjectReference, arNodeName: string, afTangentMagnitude: number, afSpeed: number, afMaxRotationSpeed: number): void;
    stopTranslation(): void;
    tetherToHorse(akHorse: ObjectReference): void;
    translateTo(afX: number, afY: number, afZ: number, afXAngle: number, afYAngle: number, afZAngle: number, afSpeed: number, afMaxRotationSpeed: number): void;
    waitForAnimationEvent(asEventName: string): boolean;
    getDistance(akOther: ObjectReference): number;
}

// Based on Actor.pex
export declare class Actor extends ObjectReference {
    static from(form: Form): Actor;
    addPerk(akPerk: Perk): void;
    addShout(akShout: Shout): boolean;
    addSpell(akSpell: Spell, abVerbose: boolean): boolean;
    allowBleedoutDialogue(abCanTalk: boolean): void;
    allowPCDialogue(abTalk: boolean): void;
    attachAshPile(akAshPileBase: Form): void;
    canFlyHere(): boolean;
    changeHeadPart(hPart: HeadPart): void;
    clearArrested(): void;
    clearExpressionOverride(): void;
    clearExtraArrows(): void;
    clearForcedMovement(): void;
    clearKeepOffsetFromActor(): void;
    clearLookAt(): void;
    damageActorValue(asValueName: string, afDamage: number): void;
    dismount(): boolean;
    dispelAllSpells(): void;
    dispelSpell(akSpell: Spell): boolean;
    doCombatSpellApply(akSpell: Spell, akTarget: ObjectReference): void;
    drawWeapon(): void;
    enableAI(abEnable: boolean): void;
    endDeferredKill(): void;
    equipItem(akItem: Form, abPreventRemoval: boolean, abSilent: boolean): void;
    equipItemById(item: Form, itemId: number, equipSlot: number, preventUnequip: boolean, equipSound: boolean): void;
    equipItemEx(item: Form, equipSlot: number, preventUnequip: boolean, equipSound: boolean): void;
    equipShout(akShout: Shout): void;
    equipSpell(akSpell: Spell, aiSource: number): void;
    evaluatePackage(): void;
    forceActorValue(asValueName: string, afNewValue: number): void;
    forceMovementDirection(afXAngle: number, afYAngle: number, afZAngle: number): void;
    forceMovementDirectionRamp(afXAngle: number, afYAngle: number, afZAngle: number, afRampTime: number): void;
    forceMovementRotationSpeed(afXMult: number, afYMult: number, afZMult: number): void;
    forceMovementRotationSpeedRamp(afXMult: number, afYMult: number, afZMult: number, afRampTime: number): void;
    forceMovementSpeed(afSpeedMult: number): void;
    forceMovementSpeedRamp(afSpeedMult: number, afRampTime: number): void;
    forceTargetAngle(afXAngle: number, afYAngle: number, afZAngle: number): void;
    forceTargetDirection(afXAngle: number, afYAngle: number, afZAngle: number): void;
    forceTargetSpeed(afSpeed: number): void;
    getActorValue(asValueName: string): number;
    getActorValueMax(asValueName: string): number;
    getActorValuePercentage(asValueName: string): number;
    getBaseActorValue(asValueName: string): number;
    getBribeAmount(): number;
    getCombatState(): number;
    getCombatTarget(): Actor;
    getCrimeFaction(): Faction;
    getCurrentPackage(): Package;
    getDialogueTarget(): Actor;
    getEquippedArmorInSlot(aiSlot: number): Armor;
    getEquippedItemId(Location: number): number;
    getEquippedItemType(aiHand: number): number;
    getEquippedObject(Location: number): Form;
    getEquippedShield(): Armor;
    getEquippedShout(): Shout;
    getEquippedSpell(aiSource: number): Spell;
    getEquippedWeapon(abLeftHand: boolean): Weapon;
    getFactionRank(akFaction: Faction): number;
    getFactionReaction(akOther: Actor): number;
    getFactions(minRank: number, maxRank: number): object[];
    getFlyingState(): number;
    getForcedLandingMarker(): ObjectReference;
    getFurnitureReference(): ObjectReference;
    getGoldAmount(): number;
    getHighestRelationshipRank(): number;
    getKiller(): Actor;
    getLevel(): number;
    getLeveledActorBase(): ActorBase;
    getLightLevel(): number;
    getLowestRelationshipRank(): number;
    getNoBleedoutRecovery(): boolean;
    getNthSpell(n: number): Spell;
    getPlayerControls(): boolean;
    getRace(): Race;
    getRelationshipRank(akOther: Actor): number;
    getSitState(): number;
    getSleepState(): number;
    getSpellCount(): number;
    getVoiceRecoveryTime(): number;
    getWarmthRating(): number;
    getWornForm(slotMask: number): Form;
    getWornItemId(slotMask: number): number;
    hasAssociation(akAssociation: Form, akOther: Actor): boolean;
    hasFamilyRelationship(akOther: Actor): boolean;
    hasLOS(akOther: ObjectReference): boolean;
    hasMagicEffect(akEffect: MagicEffect): boolean;
    hasMagicEffectWithKeyword(akKeyword: Keyword): boolean;
    hasParentRelationship(akOther: Actor): boolean;
    hasPerk(akPerk: Perk): boolean;
    hasSpell(akForm: Form): boolean;
    isAIEnabled(): boolean;
    isAlarmed(): boolean;
    isAlerted(): boolean;
    isAllowedToFly(): boolean;
    isArrested(): boolean;
    isArrestingTarget(): boolean;
    isBeingRidden(): boolean;
    isBleedingOut(): boolean;
    isBribed(): boolean;
    isChild(): boolean;
    isCommandedActor(): boolean;
    isDead(): boolean;
    isDetectedBy(akOther: Actor): boolean;
    isDoingFavor(): boolean;
    isEquipped(akItem: Form): boolean;
    isEssential(): boolean;
    isFlying(): boolean;
    isGhost(): boolean;
    isGuard(): boolean;
    isHostileToActor(akActor: Actor): boolean;
    isInCombat(): boolean;
    isInFaction(akFaction: Faction): boolean;
    isInKillMove(): boolean;
    isIntimidated(): boolean;
    isOnMount(): boolean;
    isOverEncumbered(): boolean;
    isPlayerTeammate(): boolean;
    isPlayersLastRiddenHorse(): boolean;
    isRunning(): boolean;
    isSneaking(): boolean;
    isSprinting(): boolean;
    isSwimming(): boolean;
    isTrespassing(): boolean;
    isUnconscious(): boolean;
    isWeaponDrawn(): boolean;
    keepOffsetFromActor(arTarget: Actor, afOffsetX: number, afOffsetY: number, afOffsetZ: number, afOffsetAngleX: number, afOffsetAngleY: number, afOffsetAngleZ: number, afCatchUpRadius: number, afFollowRadius: number): void;
    kill(akKiller: Actor): void;
    killSilent(akKiller: Actor): void;
    modActorValue(asValueName: string, afAmount: number): void;
    modFactionRank(akFaction: Faction, aiMod: number): void;
    moveToPackageLocation(): void;
    openInventory(abForceOpen: boolean): void;
    pathToReference(aTarget: ObjectReference, afWalkRunPercent: number): boolean;
    playIdle(akIdle: Form): boolean;
    playIdleWithTarget(akIdle: Form, akTarget: ObjectReference): boolean;
    playSubGraphAnimation(asEventName: string): void;
    queueNiNodeUpdate(): void;
    regenerateHead(): void;
    removeFromAllFactions(): void;
    removeFromFaction(akFaction: Faction): void;
    removePerk(akPerk: Perk): void;
    removeShout(akShout: Shout): boolean;
    removeSpell(akSpell: Spell): boolean;
    replaceHeadPart(oPart: HeadPart, newPart: HeadPart): void;
    resetAI(): void;
    resetExpressionOverrides(): void;
    resetHealthAndLimbs(): void;
    restoreActorValue(asValueName: string, afAmount: number): void;
    resurrect(): void;
    sendAssaultAlarm(): void;
    sendLycanthropyStateChanged(abIsWerewolf: boolean): void;
    sendTrespassAlarm(akCriminal: Actor): void;
    sendVampirismStateChanged(abIsVampire: boolean): void;
    setActorValue(asValueName: string, afValue: number): void;
    setAlert(abAlerted: boolean): void;
    setAllowFlying(abAllowed: boolean): void;
    setAllowFlyingEx(abAllowed: boolean, abAllowCrash: boolean, abAllowSearch: boolean): void;
    setAlpha(afTargetAlpha: number, abFade: boolean): void;
    setAttackActorOnSight(abAttackOnSight: boolean): void;
    setBribed(abBribe: boolean): void;
    setCrimeFaction(akFaction: Faction): void;
    setCriticalStage(aiStage: number): void;
    setDoingFavor(abDoingFavor: boolean): void;
    setDontMove(abDontMove: boolean): void;
    setExpressionModifier(index: number, value: number): void;
    setExpressionOverride(aiMood: number, aiStrength: number): void;
    setExpressionPhoneme(index: number, value: number): void;
    setEyeTexture(akNewTexture: TextureSet): void;
    setFactionRank(akFaction: Faction, aiRank: number): void;
    setForcedLandingMarker(aMarker: ObjectReference): void;
    setGhost(abIsGhost: boolean): void;
    setHeadTracking(abEnable: boolean): void;
    setIntimidated(abIntimidate: boolean): void;
    setLookAt(akTarget: ObjectReference, abPathingLookAt: boolean): void;
    setNoBleedoutRecovery(abAllowed: boolean): void;
    setNotShowOnStealthMeter(abNotShow: boolean): void;
    setOutfit(akOutfit: Outfit, abSleepOutfit: boolean): void;
    setPlayerControls(abControls: boolean): void;
    setPlayerResistingArrest(): void;
    setPlayerTeammate(abTeammate: boolean, abCanDoFavor: boolean): void;
    setRace(akRace: Race): void;
    setRelationshipRank(akOther: Actor, aiRank: number): void;
    setRestrained(abRestrained: boolean): void;
    setSubGraphFloatVariable(asVariableName: string, afValue: number): void;
    setUnconscious(abUnconscious: boolean): void;
    setVehicle(akVehicle: ObjectReference): void;
    setVoiceRecoveryTime(afTime: number): void;
    sheatheWeapon(): void;
    showBarterMenu(): void;
    showGiftMenu(abGivingGift: boolean, apFilterList: FormList, abShowStolenItems: boolean, abUseFavorPoints: boolean): number;
    startCannibal(akTarget: Actor): void;
    startCombat(akTarget: Actor): void;
    startDeferredKill(): void;
    startSneaking(): void;
    startVampireFeed(akTarget: Actor): void;
    stopCombat(): void;
    stopCombatAlarm(): void;
    trapSoul(akTarget: Actor): boolean;
    unLockOwnedDoorsInCell(): void;
    unequipAll(): void;
    unequipItem(akItem: Form, abPreventEquip: boolean, abSilent: boolean): void;
    unequipItemEx(item: Form, equipSlot: number, preventEquip: boolean): void;
    unequipItemSlot(aiSlot: number): void;
    unequipShout(akShout: Shout): void;
    unequipSpell(akSpell: Spell, aiSource: number): void;
    updateWeight(neckDelta: number): void;
    willIntimidateSucceed(): boolean;
    wornHasKeyword(akKeyword: Keyword): boolean;
}

// Based on ActorBase.pex
export declare class ActorBase extends Form {
    static from(form: Form): ActorBase;
    getClass(): Form;
    getCombatStyle(): CombatStyle;
    getDeadCount(): number;
    getFaceMorph(index: number): number;
    getFacePreset(index: number): number;
    getFaceTextureSet(): TextureSet;
    getGiftFilter(): FormList;
    getHairColor(): ColorForm;
    getHeight(): number;
    getIndexOfHeadPartByType(type: number): number;
    getIndexOfOverlayHeadPartByType(type: number): number;
    getNthHeadPart(slotPart: number): HeadPart;
    getNthOverlayHeadPart(slotPart: number): HeadPart;
    getNthSpell(n: number): Spell;
    getNumHeadParts(): number;
    getNumOverlayHeadParts(): number;
    getOutfit(bSleepOutfit: boolean): Outfit;
    getRace(): Race;
    getSex(): number;
    getSkin(): Armor;
    getSkinFar(): Armor;
    getSpellCount(): number;
    getTemplate(): ActorBase;
    getVoiceType(): Form;
    getWeight(): number;
    isEssential(): boolean;
    isInvulnerable(): boolean;
    isProtected(): boolean;
    isUnique(): boolean;
    setClass(c: Form): void;
    setCombatStyle(cs: CombatStyle): void;
    setEssential(abEssential: boolean): void;
    setFaceMorph(value: number, index: number): void;
    setFacePreset(value: number, index: number): void;
    setFaceTextureSet(textures: TextureSet): void;
    setHairColor(color: ColorForm): void;
    setHeight(height: number): void;
    setInvulnerable(abInvulnerable: boolean): void;
    setNthHeadPart(HeadPart: HeadPart, slotPart: number): void;
    setOutfit(akOutfit: Outfit, abSleepOutfit: boolean): void;
    setProtected(abProtected: boolean): void;
    setSkin(skin: Armor): void;
    setSkinFar(skin: Armor): void;
    setVoiceType(nVoice: Form): void;
    setWeight(weight: number): void;
}

// Based on ActorValueInfo.pex
export declare class ActorValueInfo extends Form {
    static from(form: Form): ActorValueInfo;
    addSkillExperience(exp: number): void;
    getBaseValue(akActor: Actor): number;
    getCurrentValue(akActor: Actor): number;
    getExperienceForLevel(currentLevel: number): number;
    getMaximumValue(akActor: Actor): number;
    getPerkTree(list: FormList, akActor: Actor, unowned: boolean, allRanks: boolean): void;
    getPerks(akActor: Actor, unowned: boolean, allRanks: boolean): object[];
    getSkillExperience(): number;
    getSkillImproveMult(): number;
    getSkillImproveOffset(): number;
    getSkillLegendaryLevel(): number;
    getSkillOffsetMult(): number;
    getSkillUseMult(): number;
    isSkill(): boolean;
    setSkillExperience(exp: number): void;
    setSkillImproveMult(value: number): void;
    setSkillImproveOffset(value: number): void;
    setSkillLegendaryLevel(level: number): void;
    setSkillOffsetMult(value: number): void;
    setSkillUseMult(value: number): void;
    static getActorValueInfoByID(id: number): ActorValueInfo;
    static getActorValueInfoByName(avName: string): ActorValueInfo;
}

// Based on Alias.pex
export declare class Alias {
    static from(form: Form): Alias;
    getID(): number;
    getName(): string;
    getOwningQuest(): Quest;
    registerForActorAction(actionType: number): void;
    registerForAnimationEvent(akSender: ObjectReference, asEventName: string): boolean;
    registerForCameraState(): void;
    registerForControl(control: string): void;
    registerForCrosshairRef(): void;
    registerForKey(keyCode: number): void;
    registerForLOS(akViewer: Actor, akTarget: ObjectReference): void;
    registerForMenu(menuName: string): void;
    registerForModEvent(eventName: string, callbackName: string): void;
    registerForNiNodeUpdate(): void;
    registerForSingleLOSGain(akViewer: Actor, akTarget: ObjectReference): void;
    registerForSingleLOSLost(akViewer: Actor, akTarget: ObjectReference): void;
    registerForSingleUpdate(afInterval: number): void;
    registerForSingleUpdateGameTime(afInterval: number): void;
    registerForSleep(): void;
    registerForTrackedStatsEvent(): void;
    registerForUpdate(afInterval: number): void;
    registerForUpdateGameTime(afInterval: number): void;
    sendModEvent(eventName: string, strArg: string, numArg: number): void;
    startObjectProfiling(): void;
    stopObjectProfiling(): void;
    unregisterForActorAction(actionType: number): void;
    unregisterForAllControls(): void;
    unregisterForAllKeys(): void;
    unregisterForAllMenus(): void;
    unregisterForAllModEvents(): void;
    unregisterForAnimationEvent(akSender: ObjectReference, asEventName: string): void;
    unregisterForCameraState(): void;
    unregisterForControl(control: string): void;
    unregisterForCrosshairRef(): void;
    unregisterForKey(keyCode: number): void;
    unregisterForLOS(akViewer: Actor, akTarget: ObjectReference): void;
    unregisterForMenu(menuName: string): void;
    unregisterForModEvent(eventName: string): void;
    unregisterForNiNodeUpdate(): void;
    unregisterForSleep(): void;
    unregisterForTrackedStatsEvent(): void;
    unregisterForUpdate(): void;
    unregisterForUpdateGameTime(): void;
}

// Based on Ammo.pex
export declare class Ammo extends Form {
    static from(form: Form): Ammo;
    getDamage(): number;
    getProjectile(): Form;
    isBolt(): boolean;
}

// Based on Armor.pex
export declare class Armor extends Form {
    static from(form: Form): Armor;
    addSlotToMask(slotMask: number): number;
    getArmorRating(): number;
    getEnchantment(): Enchantment;
    getIconPath(bFemalePath: boolean): string;
    getMessageIconPath(bFemalePath: boolean): string;
    getModelPath(bFemalePath: boolean): string;
    getNthArmorAddon(n: number): ArmorAddon;
    getNumArmorAddons(): number;
    getSlotMask(): number;
    getWarmthRating(): number;
    getWeightClass(): number;
    modArmorRating(modBy: number): void;
    removeSlotFromMask(slotMask: number): number;
    setArmorRating(armorRating: number): void;
    setEnchantment(e: Enchantment): void;
    setIconPath(path: string, bFemalePath: boolean): void;
    setMessageIconPath(path: string, bFemalePath: boolean): void;
    setModelPath(path: string, bFemalePath: boolean): void;
    setSlotMask(slotMask: number): void;
    setWeightClass(weightClass: number): void;
    static getMaskForSlot(slot: number): number;
}

// Based on ArmorAddon.pex
export declare class ArmorAddon extends Form {
    static from(form: Form): ArmorAddon;
    addSlotToMask(slotMask: number): number;
    getModelNthTextureSet(n: number, first: boolean, female: boolean): TextureSet;
    getModelNumTextureSets(first: boolean, female: boolean): number;
    getModelPath(firstPerson: boolean, female: boolean): string;
    getNthAdditionalRace(n: number): Race;
    getNumAdditionalRaces(): number;
    getSlotMask(): number;
    removeSlotFromMask(slotMask: number): number;
    setModelNthTextureSet(texture: TextureSet, n: number, first: boolean, female: boolean): void;
    setModelPath(path: string, firstPerson: boolean, female: boolean): void;
    setSlotMask(slotMask: number): void;
}

// Based on Art.pex
export declare class Art extends Form {
    static from(form: Form): Art;
    getModelPath(): string;
    setModelPath(path: string): void;
}

// Based on Book.pex
export declare class Book extends Form {
    static from(form: Form): Book;
    getSkill(): number;
    getSpell(): Spell;
    isRead(): boolean;
    isTakeable(): boolean;
}

// Based on Cell.pex
export declare class Cell extends Form {
    static from(form: Form): Cell;
    getActorOwner(): ActorBase;
    getFactionOwner(): Faction;
    getNthRef(n: number, formTypeFilter: number): ObjectReference;
    getNumRefs(formTypeFilter: number): number;
    getWaterLevel(): number;
    isAttached(): boolean;
    isInterior(): boolean;
    reset(): void;
    setActorOwner(akActor: ActorBase): void;
    setFactionOwner(akFaction: Faction): void;
    setFogColor(aiNearRed: number, aiNearGreen: number, aiNearBlue: number, aiFarRed: number, aiFarGreen: number, aiFarBlue: number): void;
    setFogPlanes(afNear: number, afFar: number): void;
    setFogPower(afPower: number): void;
    setPublic(abPublic: boolean): void;
}

// Based on ColorForm.pex
export declare class ColorForm extends Form {
    static from(form: Form): ColorForm;
    getColor(): number;
    setColor(color: number): void;
}

// Based on CombatStyle.pex
export declare class CombatStyle extends Form {
    static from(form: Form): CombatStyle;
    getAllowDualWielding(): boolean;
    getAvoidThreatChance(): number;
    getCloseRangeDuelingCircleMult(): number;
    getCloseRangeDuelingFallbackMult(): number;
    getCloseRangeFlankingFlankDistance(): number;
    getCloseRangeFlankingStalkTime(): number;
    getDefensiveMult(): number;
    getFlightDiveBombChance(): number;
    getFlightFlyingAttackChance(): number;
    getFlightHoverChance(): number;
    getGroupOffensiveMult(): number;
    getLongRangeStrafeMult(): number;
    getMagicMult(): number;
    getMeleeAttackStaggeredMult(): number;
    getMeleeBashAttackMult(): number;
    getMeleeBashMult(): number;
    getMeleeBashPowerAttackMult(): number;
    getMeleeBashRecoiledMult(): number;
    getMeleeMult(): number;
    getMeleePowerAttackBlockingMult(): number;
    getMeleePowerAttackStaggeredMult(): number;
    getMeleeSpecialAttackMult(): number;
    getOffensiveMult(): number;
    getRangedMult(): number;
    getShoutMult(): number;
    getStaffMult(): number;
    getUnarmedMult(): number;
    setAllowDualWielding(allow: boolean): void;
    setAvoidThreatChance(chance: number): void;
    setCloseRangeDuelingCircleMult(mult: number): void;
    setCloseRangeDuelingFallbackMult(mult: number): void;
    setCloseRangeFlankingFlankDistance(mult: number): void;
    setCloseRangeFlankingStalkTime(mult: number): void;
    setDefensiveMult(mult: number): void;
    setFlightDiveBombChance(chance: number): void;
    setFlightFlyingAttackChance(mult: number): void;
    setFlightHoverChance(chance: number): void;
    setGroupOffensiveMult(mult: number): void;
    setLongRangeStrafeMult(mult: number): void;
    setMagicMult(mult: number): void;
    setMeleeAttackStaggeredMult(mult: number): void;
    setMeleeBashAttackMult(mult: number): void;
    setMeleeBashMult(mult: number): void;
    setMeleeBashPowerAttackMult(mult: number): void;
    setMeleeBashRecoiledMult(mult: number): void;
    setMeleeMult(mult: number): void;
    setMeleePowerAttackBlockingMult(mult: number): void;
    setMeleePowerAttackStaggeredMult(mult: number): void;
    setMeleeSpecialAttackMult(mult: number): void;
    setOffensiveMult(mult: number): void;
    setRangedMult(mult: number): void;
    setShoutMult(mult: number): void;
    setStaffMult(mult: number): void;
    setUnarmedMult(mult: number): void;
}

// Based on Debug.pex
export declare class Debug {
    static from(form: Form): Debug;
    static centerOnCell(param1: string): void;
    static centerOnCellAndWait(param1: string): number;
    static closeUserLog(param1: string): void;
    static dBSendPlayerPosition(): void;
    static debugChannelNotify(param1: string, param2: string): void;
    static dumpAliasData(param1: Quest): void;
    static getConfigName(): string;
    static getPlatformName(): string;
    static getVersionNumber(): string;
    static messageBox(param1: string): void;
    static notification(param1: string): void;
    static openUserLog(param1: string): boolean;
    static playerMoveToAndWait(param1: string): number;
    static quitGame(): void;
    static sendAnimationEvent(param1: ObjectReference, param2: string): void;
    static setFootIK(param1: boolean): void;
    static setGodMode(param1: boolean): void;
    static showRefPosition(arRef: ObjectReference): void;
    static startScriptProfiling(param1: string): void;
    static startStackProfiling(): void;
    static stopScriptProfiling(param1: string): void;
    static stopStackProfiling(): void;
    static takeScreenshot(param1: string): void;
    static toggleAI(): void;
    static toggleCollisions(): void;
    static toggleMenus(): void;
    static trace(param1: string, param2: number): void;
    static traceStack(param1: string, param2: number): void;
    static traceUser(param1: string, param2: string, param3: number): boolean;
}

// Based on EffectShader.pex
export declare class EffectShader extends Form {
    static from(form: Form): EffectShader;
    play(param1: ObjectReference, param2: number): void;
    stop(param1: ObjectReference): void;
}

// Based on Enchantment.pex
export declare class Enchantment extends Form {
    static from(form: Form): Enchantment;
    getBaseEnchantment(): Enchantment;
    getCostliestEffectIndex(): number;
    getKeywordRestrictions(): FormList;
    getNthEffectArea(index: number): number;
    getNthEffectDuration(index: number): number;
    getNthEffectMagicEffect(index: number): MagicEffect;
    getNthEffectMagnitude(index: number): number;
    getNumEffects(): number;
    isHostile(): boolean;
    setKeywordRestrictions(newKeywordList: FormList): void;
    setNthEffectArea(index: number, value: number): void;
    setNthEffectDuration(index: number, value: number): void;
    setNthEffectMagnitude(index: number, value: number): void;
}

// Based on EquipSlot.pex
export declare class EquipSlot extends Form {
    static from(form: Form): EquipSlot;
    getNthParent(n: number): EquipSlot;
    getNumParents(): number;
}

// Based on Faction.pex
export declare class Faction extends Form {
    static from(form: Form): Faction;
    canPayCrimeGold(): boolean;
    clearFactionFlag(flag: number): void;
    getBuySellList(): FormList;
    getCrimeGold(): number;
    getCrimeGoldNonViolent(): number;
    getCrimeGoldViolent(): number;
    getInfamy(): number;
    getInfamyNonViolent(): number;
    getInfamyViolent(): number;
    getMerchantContainer(): ObjectReference;
    getReaction(akOther: Faction): number;
    getStolenItemValueCrime(): number;
    getStolenItemValueNoCrime(): number;
    getVendorEndHour(): number;
    getVendorRadius(): number;
    getVendorStartHour(): number;
    isFactionFlagSet(flag: number): boolean;
    isFactionInCrimeGroup(akOther: Faction): boolean;
    isNotSellBuy(): boolean;
    isPlayerExpelled(): boolean;
    modCrimeGold(aiAmount: number, abViolent: boolean): void;
    modReaction(akOther: Faction, aiAmount: number): void;
    onlyBuysStolenItems(): boolean;
    playerPayCrimeGold(abRemoveStolenItems: boolean, abGoToJail: boolean): void;
    sendAssaultAlarm(): void;
    sendPlayerToJail(abRemoveInventory: boolean, abRealJail: boolean): void;
    setAlly(akOther: Faction, abSelfIsFriendToOther: boolean, abOtherIsFriendToSelf: boolean): void;
    setBuySellList(akList: FormList): void;
    setCrimeGold(aiGold: number): void;
    setCrimeGoldViolent(aiGold: number): void;
    setEnemy(akOther: Faction, abSelfIsNeutralToOther: boolean, abOtherIsNeutralToSelf: boolean): void;
    setFactionFlag(flag: number): void;
    setMerchantContainer(akContainer: ObjectReference): void;
    setNotSellBuy(notSellBuy: boolean): void;
    setOnlyBuysStolenItems(onlyStolen: boolean): void;
    setPlayerEnemy(abIsEnemy: boolean): void;
    setPlayerExpelled(abIsExpelled: boolean): void;
    setReaction(akOther: Faction, aiNewValue: number): void;
    setVendorEndHour(hour: number): void;
    setVendorRadius(radius: number): void;
    setVendorStartHour(hour: number): void;
}

// Based on Flora.pex
export declare class Flora {
    static from(form: Form): Flora;
    getHarvestSound(): SoundDescriptor;
    getIngredient(): Form;
    setHarvestSound(akSoundDescriptor: SoundDescriptor): void;
    setIngredient(akIngredient: Form): void;
}

// Based on FormList.pex
export declare class FormList extends Form {
    static from(form: Form): FormList;
    addForm(apForm: Form): void;
    addForms(forms: object[]): void;
    find(apForm: Form): number;
    getAt(aiIndex: number): Form;
    getSize(): number;
    hasForm(akForm: Form): boolean;
    removeAddedForm(apForm: Form): void;
    revert(): void;
    toArray(): object[];
}

// Based on Game.pex
export declare class Game {
    static from(form: Form): Game;
    static addAchievement(aiAchievementID: number): void;
    static addHavokBallAndSocketConstraint(arRefA: ObjectReference, arRefANode: string, arRefB: ObjectReference, arRefBNode: string, afRefALocalOffsetX: number, afRefALocalOffsetY: number, afRefALocalOffsetZ: number, afRefBLocalOffsetX: number, afRefBLocalOffsetY: number, afRefBLocalOffsetZ: number): boolean;
    static addPerkPoints(aiPerkPoints: number): void;
    static advanceSkill(asSkillName: string, afMagnitude: number): void;
    static calculateFavorCost(aiFavorPrice: number): number;
    static clearPrison(): void;
    static clearTempEffects(): void;
    static disablePlayerControls(abMovement: boolean, abFighting: boolean, abCamSwitch: boolean, abLooking: boolean, abSneaking: boolean, abMenu: boolean, abActivate: boolean, abJournalTabs: boolean, aiDisablePOVType: number): void;
    static enableFastTravel(abEnable: boolean): void;
    static enablePlayerControls(abMovement: boolean, abFighting: boolean, abCamSwitch: boolean, abLooking: boolean, abSneaking: boolean, abMenu: boolean, abActivate: boolean, abJournalTabs: boolean, aiDisablePOVType: number): void;
    static fadeOutGame(abFadingOut: boolean, abBlackFade: boolean, afSecsBeforeFade: number, afFadeDuration: number): void;
    static fastTravel(akDestination: ObjectReference): void;
    static findClosestActor(afX: number, afY: number, afZ: number, afRadius: number): Actor;
    static findClosestReferenceOfAnyTypeInList(arBaseObjects: FormList, afX: number, afY: number, afZ: number, afRadius: number): ObjectReference;
    static findClosestReferenceOfType(arBaseObject: Form, afX: number, afY: number, afZ: number, afRadius: number): ObjectReference;
    static findRandomActor(afX: number, afY: number, afZ: number, afRadius: number): Actor;
    static findRandomReferenceOfAnyTypeInList(arBaseObjects: FormList, afX: number, afY: number, afZ: number, afRadius: number): ObjectReference;
    static findRandomReferenceOfType(arBaseObject: Form, afX: number, afY: number, afZ: number, afRadius: number): ObjectReference;
    static forceFirstPerson(): void;
    static forceThirdPerson(): void;
    static getCameraState(): number;
    static getCurrentConsoleRef(): ObjectReference;
    static getCurrentCrosshairRef(): ObjectReference;
    static getDialogueTarget(): ObjectReference;
    static getExperienceForLevel(currentLevel: number): number;
    static getForm(aiFormID: number): Form;
    static getFormEx(formId: number): Form;
    static getFormFromFile(aiFormID: number, asFilename: string): Form;
    static getGameSettingFloat(asGameSetting: string): number;
    static getGameSettingInt(asGameSetting: string): number;
    static getGameSettingString(asGameSetting: string): string;
    static getHotkeyBoundObject(hotkey: number): Form;
    static getLightModAuthor(idx: number): string;
    static getLightModByName(name: string): number;
    static getLightModCount(): number;
    static getLightModDependencyCount(idx: number): number;
    static getLightModDescription(idx: number): string;
    static getLightModName(idx: number): string;
    static getModAuthor(modIndex: number): string;
    static getModByName(name: string): number;
    static getModCount(): number;
    static getModDependencyCount(modIndex: number): number;
    static getModDescription(modIndex: number): string;
    static getModName(modIndex: number): string;
    static getNthLightModDependency(modIdx: number, idx: number): number;
    static getNthTintMaskColor(n: number): number;
    static getNthTintMaskTexturePath(n: number): string;
    static getNthTintMaskType(n: number): number;
    static getNumTintMasks(): number;
    static getNumTintsByType(type: number): number;
    static getPerkPoints(): number;
    static getPlayerExperience(): number;
    static getPlayerGrabbedRef(): ObjectReference;
    static getPlayerMovementMode(): boolean;
    static getPlayersLastRiddenHorse(): Actor;
    static getRealHoursPassed(): number;
    static getSunPositionX(): number;
    static getSunPositionY(): number;
    static getSunPositionZ(): number;
    static getTintMaskColor(type: number, index: number): number;
    static getTintMaskTexturePath(type: number, index: number): string;
    static hideTitleSequenceMenu(): void;
    static incrementSkill(asSkillName: string): void;
    static incrementSkillBy(asSkillName: string, aiCount: number): void;
    static incrementStat(asStatName: string, aiModAmount: number): void;
    static isActivateControlsEnabled(): boolean;
    static isCamSwitchControlsEnabled(): boolean;
    static isFastTravelControlsEnabled(): boolean;
    static isFastTravelEnabled(): boolean;
    static isFightingControlsEnabled(): boolean;
    static isJournalControlsEnabled(): boolean;
    static isLookingControlsEnabled(): boolean;
    static isMenuControlsEnabled(): boolean;
    static isMovementControlsEnabled(): boolean;
    static isObjectFavorited(Form: Form): boolean;
    static isPlayerSungazing(): boolean;
    static isPluginInstalled(name: string): boolean;
    static isSneakingControlsEnabled(): boolean;
    static isWordUnlocked(akWord: Form): boolean;
    static loadGame(name: string): void;
    static modPerkPoints(perkPoints: number): void;
    static playBink(asFilename: string, abInterruptible: boolean, abMuteAudio: boolean, abMuteMusic: boolean, abLetterbox: boolean): void;
    static precacheCharGen(): void;
    static precacheCharGenClear(): void;
    static queryStat(asStat: string): number;
    static quitToMainMenu(): void;
    static removeHavokConstraints(arFirstRef: ObjectReference, arFirstRefNodeName: string, arSecondRef: ObjectReference, arSecondRefNodeName: string): boolean;
    static requestAutosave(): void;
    static requestModel(asModelName: string): void;
    static requestSave(): void;
    static saveGame(name: string): void;
    static sendWereWolfTransformation(): void;
    static serveTime(): void;
    static setAllowFlyingMountLandingRequests(abAllow: boolean): void;
    static setBeastForm(abEntering: boolean): void;
    static setCameraTarget(arTarget: Actor): void;
    static setGameSettingBool(setting: string, value: boolean): void;
    static setGameSettingFloat(setting: string, value: number): void;
    static setGameSettingInt(setting: string, value: number): void;
    static setGameSettingString(setting: string, value: string): void;
    static setHudCartMode(abSetCartMode: boolean): void;
    static setInChargen(abDisableSaving: boolean, abDisableWaiting: boolean, abShowControlsDisabledMessage: boolean): void;
    static setMiscStat(name: string, value: number): void;
    static setNthTintMaskColor(n: number, color: number): void;
    static setNthTintMaskTexturePath(path: string, n: number): void;
    static setPerkPoints(perkPoints: number): void;
    static setPlayerAIDriven(abAIDriven: boolean): void;
    static setPlayerExperience(exp: number): void;
    static setPlayerLevel(level: number): void;
    static setPlayerReportCrime(abReportCrime: boolean): void;
    static setPlayersLastRiddenHorse(horse: Actor): void;
    static setSittingRotation(afValue: number): void;
    static setSunGazeImageSpaceModifier(apImod: ImageSpaceModifier): void;
    static setTintMaskColor(color: number, type: number, index: number): void;
    static setTintMaskTexturePath(path: string, type: number, index: number): void;
    static showFirstPersonGeometry(abShow: boolean): void;
    static showLimitedRaceMenu(): void;
    static showRaceMenu(): void;
    static showTitleSequenceMenu(): void;
    static showTrainingMenu(aTrainer: Actor): void;
    static startTitleSequence(asSequenceName: string): void;
    static teachWord(akWord: Form): void;
    static triggerScreenBlood(aiValue: number): void;
    static unbindObjectHotkey(hotkey: number): void;
    static unlockWord(akWord: Form): void;
    static updateHairColor(): void;
    static updateThirdPerson(): void;
    static updateTintMaskColors(): void;
    static usingGamepad(): boolean;
    static getPlayer(): Actor;
    static shakeCamera(akSource: ObjectReference, afStrength: number, afDuration: number): void;
    static shakeController(afSmallMotorStrength: number, afBigMotorStreangth: number, afDuration: number): void;
}

// Based on GlobalVariable.pex
export declare class GlobalVariable extends Form {
    static from(form: Form): GlobalVariable;
    getValue(): number;
    setValue(param1: number): void;
}

// Based on HeadPart.pex
export declare class HeadPart extends Form {
    static from(form: Form): HeadPart;
    getIndexOfExtraPart(p: HeadPart): number;
    getNthExtraPart(n: number): HeadPart;
    getNumExtraParts(): number;
    getPartName(): string;
    getType(): number;
    getValidRaces(): FormList;
    hasExtraPart(p: HeadPart): boolean;
    isExtraPart(): boolean;
    setValidRaces(vRaces: FormList): void;
    static getHeadPart(name: string): HeadPart;
}

// Based on ImageSpaceModifier.pex
export declare class ImageSpaceModifier extends Form {
    static from(form: Form): ImageSpaceModifier;
    apply(param1: number): void;
    applyCrossFade(param1: number): void;
    popTo(param1: ImageSpaceModifier, param2: number): void;
    remove(): void;
    static removeCrossFade(param1: number): void;
}

// Based on Ingredient.pex
export declare class Ingredient extends Form {
    static from(form: Form): Ingredient;
    getCostliestEffectIndex(): number;
    getEffectAreas(): number[];
    getEffectDurations(): number[];
    getEffectMagnitudes(): number[];
    getIsNthEffectKnown(index: number): boolean;
    getMagicEffects(): object[];
    getNthEffectArea(index: number): number;
    getNthEffectDuration(index: number): number;
    getNthEffectMagicEffect(index: number): MagicEffect;
    getNthEffectMagnitude(index: number): number;
    getNumEffects(): number;
    isHostile(): boolean;
    learnAllEffects(): void;
    learnEffect(aiIndex: number): void;
    learnNextEffect(): number;
    setNthEffectArea(index: number, value: number): void;
    setNthEffectDuration(index: number, value: number): void;
    setNthEffectMagnitude(index: number, value: number): void;
}

// Based on Keyword.pex
export declare class Keyword extends Form {
    static from(form: Form): Keyword;
    getString(): string;
    sendStoryEvent(akLoc: Location, akRef1: ObjectReference, akRef2: ObjectReference, aiValue1: number, aiValue2: number): void;
    sendStoryEventAndWait(akLoc: Location, akRef1: ObjectReference, akRef2: ObjectReference, aiValue1: number, aiValue2: number): boolean;
    static getKeyword(key: string): Keyword;
}

// Based on LeveledActor.pex
export declare class LeveledActor extends Form {
    static from(form: Form): LeveledActor;
    addForm(apForm: Form, aiLevel: number): void;
    getNthCount(n: number): number;
    getNthForm(n: number): Form;
    getNthLevel(n: number): number;
    getNumForms(): number;
    revert(): void;
    setNthCount(n: number, count: number): void;
    setNthLevel(n: number, level: number): void;
}

// Based on LeveledItem.pex
export declare class LeveledItem extends Form {
    static from(form: Form): LeveledItem;
    addForm(apForm: Form, aiLevel: number, aiCount: number): void;
    getChanceGlobal(): GlobalVariable;
    getChanceNone(): number;
    getNthCount(n: number): number;
    getNthForm(n: number): Form;
    getNthLevel(n: number): number;
    getNumForms(): number;
    revert(): void;
    setChanceGlobal(glob: GlobalVariable): void;
    setChanceNone(chance: number): void;
    setNthCount(n: number, count: number): void;
    setNthLevel(n: number, level: number): void;
}

// Based on Light.pex
export declare class Light extends Form {
    static from(form: Form): Light;
    getWarmthRating(): number;
}

// Based on Location.pex
export declare class Location extends Form {
    static from(form: Form): Location;
    getKeywordData(param1: Keyword): number;
    getRefTypeAliveCount(param1: Form): number;
    getRefTypeDeadCount(param1: Form): number;
    hasCommonParent(param1: Location, param2: Keyword): boolean;
    hasRefType(param1: Form): boolean;
    isChild(param1: Location): boolean;
    isCleared(): boolean;
    isLoaded(): boolean;
    setCleared(param1: boolean): void;
    setKeywordData(param1: Keyword, param2: number): void;
}

// Based on LocationAlias.pex
export declare class LocationAlias extends Alias {
    static from(form: Form): LocationAlias;
    clear(): void;
    forceLocationTo(param1: Location): void;
    getLocation(): Location;
}

// Based on MagicEffect.pex
export declare class MagicEffect extends Form {
    static from(form: Form): MagicEffect;
    clearEffectFlag(flag: number): void;
    getArea(): number;
    getAssociatedSkill(): string;
    getBaseCost(): number;
    getCastTime(): number;
    getCastingArt(): Art;
    getCastingType(): number;
    getDeliveryType(): number;
    getEnchantArt(): Art;
    getEnchantShader(): EffectShader;
    getEquipAbility(): Spell;
    getExplosion(): Form;
    getHitEffectArt(): Art;
    getHitShader(): EffectShader;
    getImageSpaceMod(): ImageSpaceModifier;
    getImpactDataSet(): Form;
    getLight(): Light;
    getPerk(): Perk;
    getProjectile(): Form;
    getResistance(): string;
    getSkillLevel(): number;
    getSkillUsageMult(): number;
    getSounds(): object[];
    isEffectFlagSet(flag: number): boolean;
    setArea(area: number): void;
    setAssociatedSkill(skill: string): void;
    setBaseCost(cost: number): void;
    setCastTime(castTime: number): void;
    setCastingArt(obj: Art): void;
    setEffectFlag(flag: number): void;
    setEnchantArt(obj: Art): void;
    setEnchantShader(obj: EffectShader): void;
    setEquipAbility(obj: Spell): void;
    setExplosion(obj: Form): void;
    setHitEffectArt(obj: Art): void;
    setHitShader(obj: EffectShader): void;
    setImageSpaceMod(obj: ImageSpaceModifier): void;
    setImpactDataSet(obj: Form): void;
    setLight(obj: Light): void;
    setPerk(obj: Perk): void;
    setProjectile(obj: Form): void;
    setResistance(skill: string): void;
    setSkillLevel(level: number): void;
    setSkillUsageMult(usageMult: number): void;
}

// Based on Message.pex
export declare class Message extends Form {
    static from(form: Form): Message;
    show(param1: number, param2: number, param3: number, param4: number, param5: number, param6: number, param7: number, param8: number, param9: number): number;
    showAsHelpMessage(param1: string, param2: number, param3: number, param4: number): void;
    static resetHelpMessage(param1: string): void;
}

// Based on MusicType.pex
export declare class MusicType extends Form {
    static from(form: Form): MusicType;
    add(): void;
    remove(): void;
}

// Based on Outfit.pex
export declare class Outfit extends Form {
    static from(form: Form): Outfit;
    getNthPart(n: number): Form;
    getNumParts(): number;
}

// Based on Package.pex
export declare class Package extends Form {
    static from(form: Form): Package;
    getOwningQuest(): Quest;
    getTemplate(): Package;
}

// Based on Perk.pex
export declare class Perk extends Form {
    static from(form: Form): Perk;
    getNextPerk(): Perk;
    getNthEntryLeveledList(n: number): LeveledItem;
    getNthEntryPriority(n: number): number;
    getNthEntryQuest(n: number): Quest;
    getNthEntryRank(n: number): number;
    getNthEntrySpell(n: number): Spell;
    getNthEntryStage(n: number): number;
    getNthEntryText(n: number): string;
    getNthEntryValue(n: number, i: number): number;
    getNumEntries(): number;
    setNthEntryLeveledList(n: number, lList: LeveledItem): boolean;
    setNthEntryPriority(n: number, priority: number): boolean;
    setNthEntryQuest(n: number, newQuest: Quest): boolean;
    setNthEntryRank(n: number, rank: number): boolean;
    setNthEntrySpell(n: number, newSpell: Spell): boolean;
    setNthEntryStage(n: number, stage: number): boolean;
    setNthEntryText(n: number, newText: string): boolean;
    setNthEntryValue(n: number, i: number, value: number): boolean;
}

// Based on Potion.pex
export declare class Potion extends Form {
    static from(form: Form): Potion;
    getCostliestEffectIndex(): number;
    getEffectAreas(): number[];
    getEffectDurations(): number[];
    getEffectMagnitudes(): number[];
    getMagicEffects(): object[];
    getNthEffectArea(index: number): number;
    getNthEffectDuration(index: number): number;
    getNthEffectMagicEffect(index: number): MagicEffect;
    getNthEffectMagnitude(index: number): number;
    getNumEffects(): number;
    getUseSound(): SoundDescriptor;
    isFood(): boolean;
    isHostile(): boolean;
    isPoison(): boolean;
    setNthEffectArea(index: number, value: number): void;
    setNthEffectDuration(index: number, value: number): void;
    setNthEffectMagnitude(index: number, value: number): void;
}

// Based on Quest.pex
export declare class Quest extends Form {
    static from(form: Form): Quest;
    completeAllObjectives(): void;
    completeQuest(): void;
    failAllObjectives(): void;
    getAlias(aiAliasID: number): Alias;
    getAliasById(aliasId: number): Alias;
    getAliasByName(name: string): Alias;
    getAliases(): object[];
    getCurrentStageID(): number;
    getID(): string;
    getNthAlias(index: number): Alias;
    getNumAliases(): number;
    getPriority(): number;
    isActive(): boolean;
    isCompleted(): boolean;
    isObjectiveCompleted(aiObjective: number): boolean;
    isObjectiveDisplayed(aiObjective: number): boolean;
    isObjectiveFailed(aiObjective: number): boolean;
    isRunning(): boolean;
    isStageDone(aiStage: number): boolean;
    isStarting(): boolean;
    isStopped(): boolean;
    isStopping(): boolean;
    reset(): void;
    setActive(abActive: boolean): void;
    setCurrentStageID(aiStageID: number): boolean;
    setObjectiveCompleted(aiObjective: number, abCompleted: boolean): void;
    setObjectiveDisplayed(aiObjective: number, abDisplayed: boolean, abForce: boolean): void;
    setObjectiveFailed(aiObjective: number, abFailed: boolean): void;
    start(): boolean;
    stop(): void;
    updateCurrentInstanceGlobal(aUpdateGlobal: GlobalVariable): boolean;
    static getQuest(editorId: string): Quest;
}

// Based on Race.pex
export declare class Race extends Form {
    static from(form: Form): Race;
    clearRaceFlag(n: number): void;
    getDefaultVoiceType(female: boolean): Form;
    getNthSpell(n: number): Spell;
    getSkin(): Armor;
    getSpellCount(): number;
    isRaceFlagSet(n: number): boolean;
    setDefaultVoiceType(female: boolean, voice: Form): void;
    setRaceFlag(n: number): void;
    setSkin(skin: Armor): void;
    static getNthPlayableRace(n: number): Race;
    static getNumPlayableRaces(): number;
    static getRace(editorId: string): Race;
}

// Based on ReferenceAlias.pex
export declare class ReferenceAlias extends Alias {
    static from(form: Form): ReferenceAlias;
    addInventoryEventFilter(param1: Form): void;
    clear(): void;
    forceRefTo(param1: ObjectReference): void;
    getReference(): ObjectReference;
    removeAllInventoryEventFilters(): void;
    removeInventoryEventFilter(param1: Form): void;
}

// Based on Spell.pex
export declare class Spell extends Form {
    static from(form: Form): Spell;
    cast(akSource: ObjectReference, akTarget: ObjectReference): void;
    getCastTime(): number;
    getCostliestEffectIndex(): number;
    getEffectAreas(): number[];
    getEffectDurations(): number[];
    getEffectMagnitudes(): number[];
    getEffectiveMagickaCost(caster: Actor): number;
    getEquipType(): EquipSlot;
    getMagicEffects(): object[];
    getMagickaCost(): number;
    getNthEffectArea(index: number): number;
    getNthEffectDuration(index: number): number;
    getNthEffectMagicEffect(index: number): MagicEffect;
    getNthEffectMagnitude(index: number): number;
    getNumEffects(): number;
    getPerk(): Perk;
    isHostile(): boolean;
    preload(): void;
    remoteCast(akSource: ObjectReference, akBlameActor: Actor, akTarget: ObjectReference): void;
    setEquipType(type: EquipSlot): void;
    setNthEffectArea(index: number, value: number): void;
    setNthEffectDuration(index: number, value: number): void;
    setNthEffectMagnitude(index: number, value: number): void;
    unload(): void;
}

// Based on Scene.pex
export declare class Scene extends Form {
    static from(form: Form): Scene;
    forceStart(): void;
    getOwningQuest(): Quest;
    isActionComplete(param1: number): boolean;
    isPlaying(): boolean;
    start(): void;
    stop(): void;
}

// Based on Scroll.pex
export declare class Scroll extends Form {
    static from(form: Form): Scroll;
    cast(akSource: ObjectReference, akTarget: ObjectReference): void;
    getCastTime(): number;
    getCostliestEffectIndex(): number;
    getEffectAreas(): number[];
    getEffectDurations(): number[];
    getEffectMagnitudes(): number[];
    getEquipType(): EquipSlot;
    getMagicEffects(): object[];
    getNthEffectArea(index: number): number;
    getNthEffectDuration(index: number): number;
    getNthEffectMagicEffect(index: number): MagicEffect;
    getNthEffectMagnitude(index: number): number;
    getNumEffects(): number;
    getPerk(): Perk;
    setEquipType(type: EquipSlot): void;
    setNthEffectArea(index: number, value: number): void;
    setNthEffectDuration(index: number, value: number): void;
    setNthEffectMagnitude(index: number, value: number): void;
}

// Based on ShaderParticleGeometry.pex
export declare class ShaderParticleGeometry extends Form {
    static from(form: Form): ShaderParticleGeometry;
    apply(param1: number): void;
    remove(param1: number): void;
}

// Based on Shout.pex
export declare class Shout extends Form {
    static from(form: Form): Shout;
    getNthRecoveryTime(n: number): number;
    getNthSpell(n: number): Spell;
    getNthWordOfPower(n: number): Form;
    setNthRecoveryTime(n: number, time: number): void;
    setNthSpell(n: number, aSpell: Spell): void;
    setNthWordOfPower(n: number, aWoop: Form): void;
}

// Based on SoulGem.pex
export declare class SoulGem {
    static from(form: Form): SoulGem;
    getGemSize(): number;
    getSoulSize(): number;
}

// Based on Sound.pex
export declare class Sound extends Form {
    static from(form: Form): Sound;
    getDescriptor(): SoundDescriptor;
    play(akSource: ObjectReference): number;
    playAndWait(akSource: ObjectReference): boolean;
    static setInstanceVolume(aiPlaybackInstance: number, afVolume: number): void;
    static stopInstance(aiPlaybackInstance: number): void;
}

// Based on SoundDescriptor.pex
export declare class SoundDescriptor extends Form {
    static from(form: Form): SoundDescriptor;
    getDecibelAttenuation(): number;
    getDecibelVariance(): number;
    getFrequencyShift(): number;
    getFrequencyVariance(): number;
    setDecibelAttenuation(dbAttenuation: number): void;
    setDecibelVariance(dbVariance: number): void;
    setFrequencyShift(frequencyShift: number): void;
    setFrequencyVariance(frequencyVariance: number): void;
}

// Based on TextureSet.pex
export declare class TextureSet extends Form {
    static from(form: Form): TextureSet;
    getNthTexturePath(n: number): string;
    getNumTexturePaths(): number;
    setNthTexturePath(n: number, texturePath: string): void;
}

// Based on Topic.pex
export declare class Topic extends Form {
    static from(form: Form): Topic;
    add(): void;
}

// Based on TopicInfo.pex
export declare class TopicInfo extends Form {
    static from(form: Form): TopicInfo;
    getOwningQuest(): Quest;
}

// Based on VisualEffect.pex
export declare class VisualEffect extends Form {
    static from(form: Form): VisualEffect;
    play(param1: ObjectReference, param2: number, param3: ObjectReference): void;
    stop(param1: ObjectReference): void;
}

// Based on Weapon.pex
export declare class Weapon extends Form {
    static from(form: Form): Weapon;
    fire(akSource: ObjectReference, akAmmo: Ammo): void;
    getBaseDamage(): number;
    getCritDamage(): number;
    getCritEffect(): Spell;
    getCritEffectOnDeath(): boolean;
    getCritMultiplier(): number;
    getEnchantment(): Enchantment;
    getEnchantmentValue(): number;
    getEquipType(): EquipSlot;
    getEquippedModel(): Form;
    getIconPath(): string;
    getMaxRange(): number;
    getMessageIconPath(): string;
    getMinRange(): number;
    getModelPath(): string;
    getReach(): number;
    getResist(): string;
    getSkill(): string;
    getSpeed(): number;
    getStagger(): number;
    getTemplate(): Weapon;
    getWeaponType(): number;
    setBaseDamage(damage: number): void;
    setCritDamage(damage: number): void;
    setCritEffect(ce: Spell): void;
    setCritEffectOnDeath(ceod: boolean): void;
    setCritMultiplier(crit: number): void;
    setEnchantment(e: Enchantment): void;
    setEnchantmentValue(value: number): void;
    setEquipType(type: EquipSlot): void;
    setEquippedModel(model: Form): void;
    setIconPath(path: string): void;
    setMaxRange(maxRange: number): void;
    setMessageIconPath(path: string): void;
    setMinRange(minRange: number): void;
    setModelPath(path: string): void;
    setReach(reach: number): void;
    setResist(resist: string): void;
    setSkill(skill: string): void;
    setSpeed(speed: number): void;
    setStagger(stagger: number): void;
    setWeaponType(type: number): void;
}

// Based on Weather.pex
export declare class Weather extends Form {
    static from(form: Form): Weather;
    forceActive(abOverride: boolean): void;
    getClassification(): number;
    getFogDistance(day: boolean, type: number): number;
    getSunDamage(): number;
    getSunGlare(): number;
    getWindDirection(): number;
    getWindDirectionRange(): number;
    setActive(abOverride: boolean, abAccelerate: boolean): void;
    static findWeather(auiType: number): Weather;
    static getCurrentWeather(): Weather;
    static getCurrentWeatherTransition(): number;
    static getOutgoingWeather(): Weather;
    static getSkyMode(): number;
    static releaseOverride(): void;
}

// Based on Utility.pex
export declare class Utility {
    static from(form: Form): Utility;
    static captureFrameRate(numFrames: number): string;
    static createAliasArray(size: number, fill: Alias): object[];
    static createBoolArray(size: number, fill: boolean): boolean[];
    static createFloatArray(size: number, fill: number): number[];
    static createFormArray(size: number, fill: Form): object[];
    static createIntArray(size: number, fill: number): number[];
    static createStringArray(size: number, fill: string): string[];
    static endFrameRateCapture(): void;
    static gameTimeToString(afGameTime: number): string;
    static getAverageFrameRate(): number;
    static getBudgetCount(): number;
    static getBudgetName(aiBudgetNumber: number): string;
    static getCurrentBudget(aiBudgetNumber: number): number;
    static getCurrentGameTime(): number;
    static getCurrentMemory(): number;
    static getCurrentRealTime(): number;
    static getINIBool(ini: string): boolean;
    static getINIFloat(ini: string): number;
    static getINIInt(ini: string): number;
    static getINIString(ini: string): string;
    static getMaxFrameRate(): number;
    static getMinFrameRate(): number;
    static isInMenuMode(): boolean;
    static overBudget(aiBudgetNumber: number): boolean;
    static randomFloat(afMin: number, afMax: number): number;
    static randomInt(aiMin: number, aiMax: number): number;
    static resizeAliasArray(source: object[], size: number, fill: Alias): object[];
    static resizeBoolArray(source: boolean[], size: number, fill: boolean): boolean[];
    static resizeFloatArray(source: number[], size: number, fill: number): number[];
    static resizeFormArray(source: object[], size: number, fill: Form): object[];
    static resizeIntArray(source: number[], size: number, fill: number): number[];
    static resizeStringArray(source: string[], size: number, fill: string): string[];
    static setINIBool(ini: string, value: boolean): void;
    static setINIFloat(ini: string, value: number): void;
    static setINIInt(ini: string, value: number): void;
    static setINIString(ini: string, value: string): void;
    static startFrameRateCapture(): void;
    static wait(afSeconds: number): void;
    static waitGameTime(afHours: number): void;
    static waitMenuMode(afSeconds: number): void;
}
