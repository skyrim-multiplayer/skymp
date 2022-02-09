Scriptname sweetHealthActivator extends ObjectReference  
Event OnActivate(ObjectReference akActionRef)
if !IsActivationBlocked()
Game.GetPlayer().RestoreActorValue("health", 100.0)
blockactivation()
utility.wait(2)
utility.wait(15)
blockactivation(False)
endif
endevent
