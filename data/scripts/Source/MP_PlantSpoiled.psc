Scriptname MP_PlantSpoiled extends ObjectReference

miscobject Property ItemAllowsToPlant Auto
idle Property CollectingStart Auto

float SpoilStartPoints = 0.0
bool Collected = false
bool Colleceting = false
int CollecetingTime = 0
actor actorfertilizerGlobal
Event OnActivate(ObjectReference akActionRef)
	if akActionRef.GetItemCount(ItemAllowsToPlant) != 0 && Colleceting == false
		actorfertilizerGlobal = akActionRef as actor
		Game.ForceThirdPerson()
		Game.DisablePlayerControls(false, true, true, false, true, true, true, false, 0)
		if (akActionRef as actor).IsWeaponDrawn()
			debug.Notification("Ќельз€ совершить это действие с оружием в руках")
			return
			;(akActionRef as actor).DrawWeapon()
		endif
		(akActionRef as actor).PlayIdle(CollectingStart)
		CollecetingTime = 2
		Colleceting = true
	endif
EndEvent

Event OnInit()
	SpoilStartPoints = 0
	RegisterForSingleUpdate(1.0)
EndEvent

Event OnUpdate()
	if (actorfertilizerGlobal.GetAnimationVariableBool("bInJumpState"))
		Colleceting = false
		Skymp.SetDefaultActor(actorfertilizerGlobal)
		Game.EnablePlayerControls()
	endif
	SpoilStartPoints += 1
	if (actorfertilizerGlobal.GetAnimationVariableBool("bInJumpState"))
		Colleceting = false
		Skymp.SetDefaultActor(actorfertilizerGlobal)
		Game.EnablePlayerControls()
	endif
	if CollecetingTime > 0
		CollecetingTime -= 1
	ElseIf(CollecetingTime == 0 && Colleceting == true)
		Colleceting = false
		debug.SendAnimationEvent((actorfertilizerGlobal as actor), "IdleStop")
		Skymp.SetDefaultActor(actorfertilizerGlobal)
		game.EnablePlayerControls()
		Self.Disable(true)
		Collected = true
		UnregisterForUpdate()
	endif
	if Collected == false
			RegisterForSingleUpdate(1.0)
	endif
EndEvent
