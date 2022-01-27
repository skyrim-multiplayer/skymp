Scriptname SweetScenceAnim extends ObjectReference  
int random
int count
Event OnActivate(ObjectReference akActionRef) 
random = Utility.RandomInt(1 ,6)
 Utility.RandomInt(1 ,6)
if random == 1&&count<1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleCiceroDance2")
count = count + 1
elseif random == 2&&count<1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleCiceroDance1")
count = count + 1
elseif random == 3&&count<1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleLuteStart")
count = count + 1
elseif random == 4&&count<1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleFluteStart")
count = count + 1
elseif random == 5&&count<1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleDrumStart")
count = count + 1
elseif random == 6&&count<1
count = count + 1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleBlowHornStormcloak")
elseif count >=1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleForceDefaultState")
count = 0
endif
endevent