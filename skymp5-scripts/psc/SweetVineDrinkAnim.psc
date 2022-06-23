Scriptname SweetVineDrinkAnim extends ObjectReference 
miscobject Property me  Auto    
int count
Form MyBaseObject
event oninit()
self.BlockActivation() 
GotoState("eat")
endevent
State eat
Event OnActivate(ObjectReference akActionRef) 
self.BlockActivation()
if Game.GetPlayer().GetSitState() == 0
game.getplayer().removeitem(Game.GetForm(0x8277674),10, true)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleMQ201Drink")
Game.GetPlayer().Additem(Game.GetForm(0x8253C64),1, true)
elseif Game.GetPlayer().GetSitState() == 3
Debug.SendAnimationEvent(Game.GetPlayer(),"ChairDrinkingStart")
Game.GetPlayer().RestoreActorValue("health", 50)
Game.GetPlayer().Additem(Game.GetForm(0x8253C64), 1, true)
while (game.getplayer().getitemcount(Game.GetForm(0x8253C64)) >=5)
Debug.SendAnimationEvent(Game.GetPlayer(),"idledrunkstart")
utility.wait(6)
game.getplayer().removeitem(Game.GetForm(0x8253C64), 1, true)
endwhile
endif
endevent
endstate


