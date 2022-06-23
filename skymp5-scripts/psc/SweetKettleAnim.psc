Scriptname SweetKettleAnim extends ObjectReference  
int count
event oninit()
self.BlockActivation() 
GotoState("eat")
endevent
State eat
Event OnActivate(ObjectReference akActionRef) 
self.BlockActivation() 
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
if count <1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleOffsetCarryPotServe")
count = count + 1
Game.GetPlayer().Additem(Game.GetForm(0x8277674), 1, true)
elseif count >=1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
count = 0
endif
endevent
endstate