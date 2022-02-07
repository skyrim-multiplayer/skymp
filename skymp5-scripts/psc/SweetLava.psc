Scriptname SweetLava extends ObjectReference  
Event OnTriggerEnter ( objectReference triggerRef )
	if (triggerRef == Game.GetPlayer())
		FlameDamage.Cast(triggerRef, triggerRef)
	endIf
endEvent