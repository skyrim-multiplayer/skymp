Scriptname SweetMirrorActivator extends ObjectReference  

Event OnActivate(ObjectReference akActionRef)
	Game.ForceThirdPerson()
	Utility.Wait(0.2)
	Game.ShowRaceMenu()
EndEvent
