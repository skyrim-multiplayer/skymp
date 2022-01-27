Scriptname SweetEatSoup extends ObjectReference   
int count
event oninit()
SetMotionType(4)
self.BlockActivation() 
GotoState("eat")
endevent
State eat
Event OnActivate(ObjectReference akActionRef) 
if count < 1&&Game.GetPlayer().getitemcount(Game.GetForm(0x6277674)) < 1
self.BlockActivation() 
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleEatingStandingStart")
Game.GetPlayer().RestoreActorValue("health", 50)
count = count + 1
elseif count >= 1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
game.getplayer().removeitem(Game.GetForm(0x6277674), 10)
count = 0
elseif  Game.GetPlayer().getitemcount(Game.GetForm(0x6277674)) >= 1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleOffsetCarryPotServe")
endif
endevent
endstate
 
