Scriptname MP_PlantGrowth extends ObjectReference

miscobject Property ItemAllowsToPlant Auto
miscobject Property EmptyBucket Auto
miscobject Property BucketWithWater Auto
miscobject Property fertilizer Auto
Activator Property SpoiledPlantActivator Auto
Activator Property GrownPlantActivator Auto
message Property CareMessage Auto
formlist Property ListOfotherPlants Auto
idle Property WateringStart Auto
Idle Property weedStart Auto
idle Property fertilizeStart Auto

float Property NeedtimeToGrowth = 60.0 auto
Float Property waterPointsGain Auto
Float[] Property waterPointsOptimal Auto
Float[] Property waterPointsNeed Auto
Float Property weedPointsGain Auto
float Property fertilizePointsGain Auto
Float[] Property weedPointsOptimal Auto
Float Property goodWateringGrowthCoef = 0.1 Auto
Float Property goodFertilizeGrowthCoef = 0.2 Auto
Float Property CloseOtherPlantGrowthCoef = 0.5 Auto
Float Property RadiusForOtherPlant Auto

actor lastActivator
float GrowthStartPoints = 0.0
float GoodCareBonus = 1.0
Float WaterPoints = 0.0
Float weedPoints = 0.0
Float fertilizePoints = 0.0
Bool PlantBusy = false
bool watering = false
int wateringTimer = 0
bool weeding = false
int weedingTimer = 0
bool Fertilizing = false
int FertilizingTimer = 0
bool AlreadyGrown = false
bool Spoiled = false
bool WeededFirsTime = false
Bool WateredFirstTime = false
int lastTimeWatered = 0
int lastTimeWeeded = 0
int lastTimeFertilized = 0

Event OnActivate(ObjectReference akActionRef)
	if (akActionRef.GetItemCount(ItemAllowsToPlant) != 0) && (PlantBusy == false)
		lastActivator = akActionRef as actor
		int PercentOfGroth = ((GrowthStartPoints/NeedtimeToGrowth) * 100) as int
		int Button = CareMessage.Show(PercentOfGroth, (lastTimeWatered) as int, (lastTimeWeeded) as int, (lastTimeFertilized) as  int)
		;Debug.Notification("" + lastActivator.IsWeaponDrawn())
		if (Button > 0 && lastActivator.IsWeaponDrawn())
			debug.Notification("Ќельз€ совершить это действие с оружием в руках")
			return
		endif
		PlantBusy = true
		if Button == 1
			wateringPlant(akActionRef)
		ElseIf(Button == 2)
			weedPlant(akActionRef)
		ElseIf(Button == 3)
			fertilizePlant(akActionRef)
		else
			PlantBusy = false
		endif
	endif
EndEvent

Event OnInit()
	self.SetAngle(0, 0, 0)
	RegisterForSingleUpdate(1.0)
EndEvent

Event OnUpdate()
	updateGrowth()
	CheckProcesses()
	if AlreadyGrown == false && Spoiled == false
		RegisterForSingleUpdate(1.0)
	endif
EndEvent

float Function GetRangeToOtherPlant()
	if (game.FindClosestReferenceOfAnyTypeInListFromRef(ListOfotherPlants, Self as objectReference, RadiusForOtherPlant) == none)
		Return 1.0
	else
		return CloseOtherPlantGrowthCoef
	endif
EndFunction

Float Function getGoodWateringCoef()
	if WaterPoints > waterPointsOptimal[0] && WaterPoints < waterPointsOptimal[1]
		 return goodWateringGrowthCoef
	else
		return 0.0
	endif
EndFunction

Float Function getFertilizeCoef()
	if fertilizePoints > 0
		Return goodfertilizeGrowthCoef
	else
		return 0.0
	endif
EndFunction

Function updateGrowth()
	if WateredFirstTime
		WaterPoints -= 1
	endif
	if fertilizePoints > 0
		fertilizePoints -= 1
	endif
	if WeededFirsTime
		weedPoints -= 1
	endif
	lastTimeWatered += 1
	lastTimeWeeded += 1
	lastTimeFertilized += 1
	GrowthStartPoints += 1 * (1 + (getGoodWateringCoef() + getFertilizeCoef())) * GetRangeToOtherPlant()
	if (((WaterPoints > waterPointsNeed[1] || WaterPoints < waterPointsNeed[0]) || (weedPoints < weedPointsOptimal[0] || weedPoints > weedPointsOptimal[1])) && Spoiled == false) && !self.IsDisabled()
		Self.PlaceAtMe(SpoiledPlantActivator, 1, false, false)
		Spoiled = true
		Self.Disable(true)
	endif
	if (GrowthStartPoints >= NeedtimeToGrowth) && (Spoiled == false) && !(AlreadyGrown)
		Self.PlaceAtMe(GrownPlantActivator, 1, false, false)
		AlreadyGrown = true
		Self.Disable(true)
	endif
EndFunction

Function CheckProcesses()
	if wateringTimer > 0
		wateringTimer -= 1
	endif
	if FertilizingTimer > 0
		FertilizingTimer -= 1
	endif
	if weedingTimer > 0
		weedingTimer -= 1
	endif
	if (lastActivator.GetAnimationVariableBool("bInJumpState"))
		watering = false
		Fertilizing = false
		weeding = false
		PlantBusy = false
		Skymp.SetDefaultActor(lastActivator)
		Game.EnablePlayerControls()
	endif
	if (wateringTimer == 0 && watering == true)
		watering = false
		debug.SendAnimationEvent((actorWatererGlobal), "IdleStop")
		WaterPoints += waterPointsGain
		WateredFirstTime = true
		lastTimeWatered = 0
		PlantBusy = false
		Skymp.SetDefaultActor(actorWatererGlobal)
		Game.EnablePlayerControls()
	endif
	if (FertilizingTimer == 0 && Fertilizing == true)
		Fertilizing = false
		lastTimeFertilized = 0
		debug.SendAnimationEvent((actorfertilizerGlobal), "IdleStop")
		fertilizePoints += fertilizePointsGain
		PlantBusy = false
		Skymp.SetDefaultActor(actorfertilizerGlobal)
		Game.EnablePlayerControls()
	endif
	if (weedingTimer == 0 && weeding == true)
		weeding = false
		WeededFirsTime = true
		lastTimeWeeded = 0
		debug.SendAnimationEvent((actorWeederGlobal), "IdleStop")
		WeedPoints += weedPointsGain
		PlantBusy = false
		Skymp.SetDefaultActor(actorWeederGlobal)
		Game.EnablePlayerControls()
	endif
EndFunction

actor actorWatererGlobal
actor actorWeederGlobal
actor actorfertilizerGlobal

Function weedPlant(ObjectReference actorWaterer)
	actorWeederGlobal = actorWaterer as actor
	Skymp.SetDefaultActor(actorWeederGlobal)
	Game.ForceThirdPerson()
	Game.DisablePlayerControls(false, true, true, false, true, true, true, false, 0)
	if (actorWeederGlobal as actor).IsWeaponDrawn()
		debug.Notification("Ќельз€ совершить это действие с оружием в руках")
		return
		;(actorWeederGlobal as actor).DrawWeapon()
	endif
	(actorWaterer as actor).PlayIdle(weedStart)
 	weedingTimer = 5
	weeding = true
EndFunction

Function fertilizePlant(ObjectReference actorWaterer)
	actorfertilizerGlobal = actorWaterer as actor
	Skymp.SetDefaultActor(actorfertilizerGlobal)
	Game.ForceThirdPerson()
	Game.DisablePlayerControls(false, true, true, false, true, true, true, false, 0)
	if (actorfertilizerGlobal as actor).IsWeaponDrawn()
		debug.Notification("Ќельз€ совершить это действие с оружием в руках")
		return
		;(actorfertilizerGlobal as actor).DrawWeapon()
	endif
	(actorWaterer as actor).PlayIdle(fertilizeStart)
	(actorWaterer).RemoveItem(fertilizer, 1, false, none)
	;Debug.Notification("”добрение использовано")
	fertilizingTimer = 3
	Fertilizing = true
EndFunction

Function wateringPlant(ObjectReference actorWaterer)
	actorWatererGlobal = actorWaterer as actor
	Skymp.SetDefaultActor(actorWatererGlobal)
	Game.ForceThirdPerson()
	Game.DisablePlayerControls(false, true, true, false, true, true, true, false, 0)
	if (actorWatererGlobal as actor).IsWeaponDrawn()
		debug.Notification("Ќельз€ совершить это действие с оружием в руках")
		return
		;(actorWatererGlobal as actor).DrawWeapon()
	endif
	(actorWaterer as actor).PlayIdle(WateringStart)
	(actorWaterer).RemoveItem(BucketWithWater, 1, false, none)
	(actorWaterer).additem(EmptyBucket, 1, false)
	;Debug.Notification("Ѕадь€ опустошена")
	wateringTimer = 3
	watering = true
EndFunction
