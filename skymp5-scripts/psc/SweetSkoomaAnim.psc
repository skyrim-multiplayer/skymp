Scriptname SweetSkoomaAnim extends ObjectReference  
int count
event oninit()
self.BlockActivation() 
GotoState("eat")
endevent
State eat
Event OnActivate(ObjectReference akActionRef) 
self.BlockActivation() 
game.getplayer().removeitem(Game.GetForm(0x6277674),10, true)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
if Game.GetPlayer().GetSitState() == 0
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleDrinkPotion")
Game.GetPlayer().RestoreActorValue("health", 50)
utility.wait(6)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleDrunk_Walk")
elseif Game.GetPlayer().GetSitState() == 3
Debug.SendAnimationEvent(Game.GetPlayer(),"ChairDrinkingStart")
endif
endevent
endstate
