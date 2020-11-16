Scriptname MP_PlantGrown extends ObjectReference

int Property ProductCount Auto
potion Property Product Auto
potion Property Seed Auto
miscobject Property ItemAllowsToPlant Auto
float Property NeedtimeToSpoil = 60.0 auto
idle Property CollectingStart Auto
Activator Property SpoiledPlantActivator Auto

float SpoilStartPoints = 0.0
bool Collected = false
bool Colleceting = false
int CollecetingTime = 0

Actor ACtorGlobal

Event OnActivate(ObjectReference akActionRef)
	if akActionRef.GetItemCount(ItemAllowsToPlant) != 0
		ACtorGlobal = akActionRef as actor
		if (ACtorGlobal.IsWeaponDrawn())
			debug.Notification("Нельзя совершить это действие с оружием в руках")
			return
		endif
		Game.ForceThirdPerson()
		Game.DisablePlayerControls(false, true, true, false, true, true, true, false, 0)
		(akActionRef as actor).PlayIdle(CollectingStart)
		CollecetingTime = 1
		Colleceting = true
	endif
EndEvent

Event OnInit()
	SpoilStartPoints = 0
	RegisterForSingleUpdate(1.0)
EndEvent

Event OnUpdate()
	SpoilStartPoints += 1
	if (ACtorGlobal.GetAnimationVariableBool("bInJumpState"))
		Colleceting = false
		Skymp.SetDefaultActor(ACtorGlobal)
		Game.EnablePlayerControls()
	endif
	if CollecetingTime > 0
		CollecetingTime -= 1
	ElseIf(CollecetingTime == 0 && Colleceting == true)
		Colleceting = false
		ACtorGlobal.AddItem(Product, ProductCount, false)
		Self.Disable(true)
		Collected = true
		UnregisterForUpdate()
		ACtorGlobal.AddItem(Seed, 1, false)
		;Debug.Notification("пїЅпїЅпїЅпїЅпїЅпїЅпїЅ: " + Product.GetName() + " - " + ProductCount + " пїЅпїЅ.")
		Skymp.SetDefaultActor(ACtorGlobal)
		Game.EnablePlayerControls()
	endif
	if SpoilStartPoints >= NeedtimeToSpoil && Collected == false
		Collected = true
	 	self.PlaceAtMe(SpoiledPlantActivator, 1, false, false)
	 	Self.Disable(true)
	else
		if Collected == false
			RegisterForSingleUpdate(1.0)
		endif
	endif
EndEvent
