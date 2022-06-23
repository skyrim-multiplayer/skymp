Scriptname SweetPlateAnim extends ObjectReference  
int count
event oninit()
SetMotionType(4)
self.BlockActivation() 
GotoState("eat")
endevent
State eat
Event OnActivate(ObjectReference akActionRef) 
self.BlockActivation() 
endevent
endstate