Scriptname sweetAnimWash extends ObjectReference  
Event OnActivate(ObjectReference akActionRef) 
Game.GetPlayer().UnequipAll()
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleWipeBrow")
utility.wait(2)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleRitualSkull1")
utility.wait(4)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleRitualSkull2")
utility.wait(5)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleRitualSkull3")
utility.wait(6)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleWarmArms")
EndEvent