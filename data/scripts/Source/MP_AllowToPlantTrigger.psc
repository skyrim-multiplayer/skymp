Scriptname MP_allowToplanttrigger extends ObjectReference

miscobject Property ItemAllowsToPlant Auto
miscobject Property ItemAllowsEnter Auto

auto STATE waitingForActor
	EVENT onTriggerEnter(objectReference triggerRef)
		if (triggerRef).GetItemCount(ItemAllowsEnter) !=0
			AllowPlant(triggerRef)
			debug.Notification("Вы вошли в зону посадки")
		endif
	endEVENT

	EVENT onTriggerleave(objectReference triggerRef)
		if (triggerRef).GetItemCount(ItemAllowsEnter) !=0
			debug.Notification("Вы покинули зону посадки")
			DisallowPlant(triggerRef)
		endif
	endEVENT
endSTATE


Function AllowPlant(objectReference ActorCan)
	if ActorCan.GetItemCount(ItemAllowsToPlant) <= 0
		ActorCan.AddItem(ItemAllowsToPlant, 1, true)
	endif
EndFunction

Function DisallowPlant(objectReference ActorCan)
	if ActorCan.GetItemCount(ItemAllowsToPlant) > 0
		ActorCan.RemoveItem(ItemAllowsToPlant, 1, true)
	endif
EndFunction
