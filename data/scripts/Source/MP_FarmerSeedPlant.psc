Scriptname MP_FarmerSeedPlant extends Actor

miscobject Property ItemAllowsToPlant Auto
Activator[] Property PlantActivator Auto
idle Property plantStart Auto
Potion[] Property seedItem Auto
formlist Property ListOfotherPlants Auto

int lastSeedUsed = 0
bool planting = false
int plantingTimer = 0

int secondsCounter = 0
int lastPlaceAtMe = -1

Event OnObjectEquipped(Form akBaseObject, ObjectReference akReference)
	int i = 0
	bool found = false
	While(i < seedItem.Length && !(akBaseObject == seedItem[i]))
		i += 1
	EndWhile
	If (i == seedItem.Length)
		return
	EndIf
	; Removed !(planting) condition
	if ((Self).GetItemCount(ItemAllowsToPlant) != 0 ) && !((Self).GetAnimationVariableBool("bInJumpState")) && (game.FindClosestReferenceOfAnyTypeInListFromRef(ListOfotherPlants, Self as objectReference, 50) == none)
		lastSeedUsed = i
		Plant()
		planting = true
		Utility.Wait(1.5)
		debug.SendAnimationEvent(((Self as actor)), "IdleStop")
		Game.EnablePlayerControls()
		(Self).PlaceAtMe(PlantActivator[lastSeedUsed], 1, true, false)
		planting = false
	else
		(Self).AddItem(seedItem[i], 1, true)
		debug.Notification("Здесь нельзя садить")
	endif
EndEvent

Event OnInit()
	RegisterForSingleUpdate(1.0)
EndEvent

Function Plant()
	Game.ForceThirdPerson()
	Game.DisablePlayerControls(false, true, true, false, true, true, true, false, 0)
	plantingTimer = 1
	planting = true
	if (Self as actor).IsWeaponDrawn()
		debug.Notification("Нельзя совершить это действие с оружием в руках")
		return
		;(Self as actor).DrawWeapon()
	endif
	(Self as actor).PlayIdle(plantStart)
;	debug.SendAnimationEvent((ACtorPlant as actor), "IdleStop")
EndFunction

Event OnUpdateeeee()
	secondsCounter = secondsCounter + 1
	if (secondsCounter == 300)
		secondsCounter = 0
	endif
	if ((Self).GetAnimationVariableBool("bInJumpState"))
		planting = false
		Game.EnablePlayerControls()
	endif
	if plantingTimer > 0
		plantingTimer -= 1
	endif
	if plantingTimer == 0 && planting == true
		debug.SendAnimationEvent(((Self as actor)), "IdleStop")
		Game.EnablePlayerControls()
		planting = false
		if (secondsCounter != lastPlaceAtMe)
			lastPlaceAtMe = secondsCounter
			(Self).PlaceAtMe(PlantActivator[lastSeedUsed], 1, true, false)
		endif
	endif
	RegisterForSingleUpdate(1.0)
EndEvent
