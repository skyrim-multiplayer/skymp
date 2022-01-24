Scriptname SweetWriteShipAnim extends ObjectReference  
int count
Event OnActivate(ObjectReference akActionRef)
if count < 1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleWriteLedgerEnter")
count = count + 1
elseif count >=1
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleWriteLedgerWrite")
count = 0
endif 
endevent