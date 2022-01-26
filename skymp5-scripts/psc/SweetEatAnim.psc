Scriptname SweetEatAnim extends ObjectReference  
int count
State eat
Event OnActivate(ObjectReference akActionRef) 
if count < 1&&Game.GetPlayer().GetSitState() == 0
game.getplayer().removeitem(Game.GetForm(0x6277674),10, true)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
self.BlockActivation() 
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleEatingStandingStart")
count = count + 1
elseif  count < 1&&Game.GetPlayer().GetSitState() == 3
Debug.SendAnimationEvent(Game.GetPlayer(),"ChairEatingStart")
Game.GetPlayer().RestoreActorValue("health", 50)
count= count + 1
elseif count >= 1&&!Game.GetPlayer().GetSitState() == 3
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
count = 0
elseif count >= 1
Debug.SendAnimationEvent(Game.GetPlayer(),"ChairEatingStart")
Game.GetPlayer().RestoreActorValue("health", 50)
count = 0
endif
endevent
endstate
