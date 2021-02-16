ScriptName aaMP_RestFurniture extends ObjectReference  

float property _fatigueRemoveMultiplier = 1.0 auto

float furnitureUpdateTimeUnit = 1.0
Actor currentActor

event onActivate(ObjectReference akActionRef)
	if (!currentActor)
		currentActor = akActionRef as Actor
		(akActionRef as aaMP_PlayerFatigue).modFatigueRemoveMultiplier(_fatigueRemoveMultiplier)
		registerForSingleUpdate(furnitureUpdateTimeUnit)
	endif
endevent

event onUpdate()   
	if (!isFurnitureMarkerInUse(0))	
		(currentActor as aaMP_PlayerFatigue).modFatigueRemoveMultiplier(-_fatigueRemoveMultiplier)
		currentActor= none
		unregisterForUpdate()
	else
		registerForSingleUpdate(furnitureUpdateTimeUnit)
	endif
endevent