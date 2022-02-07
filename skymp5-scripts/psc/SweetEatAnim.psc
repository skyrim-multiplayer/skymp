Scriptname SweetEatAnim extends ObjectReference  
int count
event oninit()
self.BlockActivation() 
GotoState("eat")
endevent
State eat
Event OnActivate(ObjectReference akActionRef) 
count = count + 1
if count == 2
self.BlockActivation()
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
count = 0
elseif Game.GetPlayer().GetSitState() == 0 
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleEatingStandingStart")
Game.GetPlayer().RestoreActorValue("health", 50)
elseif Game.GetPlayer().GetSitState() == 3
Debug.SendAnimationEvent(Game.GetPlayer(),"ChairEatingStart")
Game.GetPlayer().RestoreActorValue("health", 50)
endif
endevent
endstate
