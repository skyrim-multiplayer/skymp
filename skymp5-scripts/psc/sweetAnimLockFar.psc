Scriptname sweetAnimLockFar extends ObjectReference  
int count
Event OnActivate(ObjectReference akActionRef)
if count < 1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleLookFar")
count = count + 1
elseif count >=1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
count = 0
endif
endevent