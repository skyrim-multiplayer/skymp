Scriptname SweetScenceAnim2 extends ObjectReference
int random
int count
Event OnActivate(ObjectReference akActionRef) 
random = Utility.RandomInt(1 ,2)
 Utility.RandomInt(1 ,2)
if random == 1&&count<1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleSitCrossLeggedEnter")
count = count + 1
elseif random == 2&&count<1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleChildSitOnKnees")
elseif count >=1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
count = 0
endif
endevent