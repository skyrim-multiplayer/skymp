Scriptname SweetPotionAnim extends ObjectReference  
event oninit()
SetMotionType(4)
self.BlockActivation() 
GotoState("eat")
endevent
State eat
Event OnActivate(ObjectReference akActionRef) 
self.BlockActivation() 
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleDrinkPotion")
endevent
endstate