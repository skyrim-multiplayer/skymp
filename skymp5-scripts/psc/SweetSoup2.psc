Scriptname SweetSoup2 extends ObjectReference  
int count
event oninit()
self.BlockActivation() 
GotoState("eat")
endevent
State eat
Event OnActivate(ObjectReference akActionRef) 
if count < 1&&Game.GetPlayer().GetSitState() == 0&&  Game.GetPlayer().getitemcount(Game.GetForm(0x8277674)) < 1
self.BlockActivation() 
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleEatingStandingStart")
count = count + 1
elseif  count < 1&&Game.GetPlayer().GetSitState() == 3
Debug.SendAnimationEvent(Game.GetPlayer(),"ChairEatingSoupStart")
utility.wait(3)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleEatSoup")
count= count + 1
elseif count >= 1&&!Game.GetPlayer().GetSitState() == 3
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
count = 0
elseif count >= 1
Debug.SendAnimationEvent(Game.GetPlayer(),"ChairEatingSoupStop")
count = 0
elseif  Game.GetPlayer().getitemcount(Game.GetForm(0x8277674)) >= 1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleOffsetCarryPotServe")
endif
endevent
endstate