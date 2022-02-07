Scriptname SweetSitAnim extends ObjectReference  
int count
int random
Event OnActivate(ObjectReference akActionRef)
random = Utility.RandomInt(1 ,2)

if random == 1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleSitCrossLeggedEnter")
elseif random == 2
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleChildSitOnKnees")
endif
endevent

