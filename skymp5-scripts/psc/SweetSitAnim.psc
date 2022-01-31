Scriptname SweetSitAnim extends ObjectReference  
Int random
Event OnActivate(ObjectReference akActionRef)
random = Utility.RandomInt(1 ,3)
 Utility.RandomInt(1 ,3)
if random == 1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleSitCrossLeggedEnter")
elseif random == 2
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleGreybeardMeditateEnter")
elseif random == 3
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleChildSitOnKnees")
endif
endevent

