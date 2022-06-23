Scriptname SweetCampFireAnim extends ObjectReference  
Int random
int count
Event OnActivate(ObjectReference akActionRef)
random = Utility.RandomInt(1 ,2)
 Utility.RandomInt(1 ,2)
if random == 1&&count<1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleWarmHandsStanding")
count = count + 1
elseif random == 2&&count<1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleWarmHandsCrouched")
count = count + 1
elseif count >=1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
count = 0
endif
endevent

